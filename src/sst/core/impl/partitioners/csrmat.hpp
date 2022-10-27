#ifndef SST_CORE_IMPL_PARTITONERS_CSRMAT_HPP
#define SST_CORE_IMPL_PARTITONERS_CSRMAT_HPP

#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>
#include <unordered_set>
#include <unordered_map>

#include "sst/core/impl/partitioners/util.h"
#include <metis.h>

  class CSRMat {
  public:
    // construct from components
    CSRMat(std::unordered_map<int, std::vector<double>> nw, std::unordered_map<std::pair<int, int>, double> ew)
        : _edges{}, _nodes{}, _node_wgts_map(std::move(nw)), _edge_wgts_map(std::move(ew)) {
        if (_node_wgts_map.size() > 0) {
            _constraint_number = _node_wgts_map.cbegin()->second.size();
        }
        for (const auto& p : _node_wgts_map) {
            _nodes.push_back(p.first);

            // Enforce that each vector-value in the node weight map has the same length.
            if (p.second.size() != _constraint_number) {
                std::string err_msg =
                    "Error: Two nodes with different number of constraints\n"
                    "    Node " +
                    std::to_string(_node_wgts_map.cbegin()->first) + " has constraints { ";
                for (const double& c : _node_wgts_map.cbegin()->second) {
                    err_msg += std::to_string(c) + " ";
                }
                err_msg += "}\n";
                err_msg += "    Node " + std::to_string(p.first) + " has constraints { ";
                for (const double& c : p.second) {
                    err_msg += std::to_string(c) + " ";
                }
                err_msg += "}\n";
                throw std::logic_error(err_msg);
            }
        }
        std::sort(_nodes.begin(), _nodes.end());

        // use of intermediate edge_sets normalizes all edges to be
        // bi-directional
        std::unordered_map<int, std::unordered_set<int>> edge_sets;
        for (const auto& p : _edge_wgts_map) {
            int src = p.first.first, dst = p.first.second;
            edge_sets[src].insert(dst);
            edge_sets[dst].insert(src);
        }
        for (const auto& p : edge_sets) {
            int src = p.first;
            for (int dst : p.second) {
                _edges[src].push_back(dst);
            }
            std::sort(_edges[src].begin(), _edges[src].end());
        }
    }

    size_t size() const { return _nodes.size(); }
    const std::vector<int>& node_ID() const { return _nodes; }
    //maps flattend vertex indices to vertex id
    int get(unsigned int index) const { return _nodes[index]; }
    std::vector<double> node_weight(int id) const { return _node_wgts_map.at(id); }

    std::vector<int> xadj() const {
        std::vector<int> result;
        result.push_back(0);
        for (auto id : _nodes) {
            int edge_n = _edges.count(id) ? _edges.at(id).size() : 0;
            int idx    = result.back() + edge_n;
            result.push_back(idx);
        }
        return result;
    }

    std::vector<int> adj() const {
        std::vector<int> result;
        // mapping from node IDs to index [0, num_nodes)
        std::unordered_map<int, int> mapping;
        for (unsigned i = 0; i < _nodes.size(); ++i) {
            mapping[_nodes[i]] = i;
        }
        for (auto src : _nodes) {
            if (_edges.count(src)) {
                for (auto dst : _edges.at(src)) {
                    result.push_back(mapping.at(dst));
                }
            }
        }
        return result;
    }

    std::size_t constraint_number() const { return _constraint_number; }

    // vector of node weights in order
    std::vector<double> node_wgts() const {
        std::vector<double> result;
        result.reserve(_constraint_number * size());  // pre-allocate
        for (auto id : _nodes) {                      // _nodes is sorted
            for (uint c = 0; c < _constraint_number; ++c) {
                result.push_back(_node_wgts_map.at(id).at(c));
            }
        }
        return result;
    }

    // returns a vector of edge weights in order
    std::vector<double> edge_wgts() const {
        std::vector<double> result;
        for (auto src : _nodes) {                  // _nodes is sorted
            if (_edges.count(src)) {               // node might not be connected to anything
                for (auto dst : _edges.at(src)) {  // _edges.at(src) is sorted
                    double w{0};
                    auto id = std::make_pair(src, dst);
                    if (_edge_wgts_map.count(id)) {
                        w += _edge_wgts_map.at(id);
                    }
                    id = std::make_pair(dst, src);
                    if (_edge_wgts_map.count(id)) {
                        w += _edge_wgts_map.at(id);
                    }
                    result.push_back(w);
                }
            }
        }
        return result;
    }

    std::vector<int> idxs_to_node_ID(std::vector<int> idxs) const {
        std::vector<int> result;
        for (auto idx : idxs) {
            result.push_back(_nodes[idx]);
        }
        return result;
    }

    friend std::ostream& operator<<(std::ostream& os, const CSRMat& mat) {
        os << "CSRMat (" << mat.size() << ")\n";
        auto nw  = mat.node_wgts();
        auto ew  = mat.edge_wgts();
        int nidx = 0, eidx = 0;
        for (auto nid : mat._nodes) {
            os << "(" << nid << "," << nw[nidx++] << ") : ";
            for (auto eid : mat._edges.at(nid)) {
                os << "(" << eid << "," << ew[eidx++] << "),";
            }
            os << '\n';
        }
        return os;
    }

    void csr_info() {
        std::cout << "size() " << _nodes.size() << std::endl;

        std::cout << "First ten elements of _nodes \n";
        for (int j = 0; j < 10; ++j)
            std::cout << _nodes[j] << std::endl;

        std::cout << "First ten elements of _edges \n";
        for (int j = 0; j < 10; ++j) {
            for (int k : _edges[j])
                std::cout << k << " ";
            std::cout << "\n";
        }
    }

    std::unordered_map<int, std::vector<int>> get_edges() { return _edges; }

  private:
    std::size_t _constraint_number;
    std::unordered_map<int, std::vector<int>> _edges;
    std::vector<int> _nodes;  // sorted node IDs
    std::unordered_map<int, std::vector<double>> _node_wgts_map;
    std::unordered_map<std::pair<int, int>, double> _edge_wgts_map;
};

template <class T>
std::vector<int64_t> to_int64(const std::vector<T>& vec) {
    std::vector<int64_t> result;
    result.reserve(vec.size());
    for (auto val : vec) {
        result.push_back(static_cast<int64_t>(val));
    }
    return result;
}

template <class T>
std::vector<int64_t> scale_to_int64(const std::vector<T>& vec) {
    double max_aval = 0;
    for (auto val : vec) {
        if (fabs(val) > max_aval)
            max_aval = fabs(val);
    }
    double target       = std::sqrt(double(std::numeric_limits<int64_t>::max()));
    double scale_factor = target / max_aval;
    std::vector<int64_t> result;
    result.reserve(vec.size());
    for (auto val : vec) {
        result.push_back(static_cast<int64_t>(scale_factor * val));
    }
    return result;
}

template <class T>
void summarize_vec(std::vector<T> vec) {
    std::cout << "size=" << vec.size();
    auto min_v = std::numeric_limits<T>::max();
    auto max_v = std::numeric_limits<T>::min();
    for (const auto& v : vec) {
        min_v = std::min(v, min_v);
        max_v = std::max(v, max_v);
    }
    std::cout << ", min=" << min_v;
    std::cout << ", max=" << max_v;
    std::cout << ", (";
    for (size_t i = 0; i < std::min(4ul, vec.size()); ++i) {
        std::cout << vec[i] << ", ";
    }
    std::cout << "..., ";
    for (size_t i = std::max(0ul, vec.size() - 4); i < vec.size(); ++i) {
        std::cout << vec[i] << ", ";
    }
    std::cout << "\b\b)";
}

std::vector<int64_t> metis_part(const CSRMat& mat, int64_t nparts, const double imba_ratio) {
    static_assert(sizeof(idx_t) == sizeof(int64_t), "Requires 64-bit METIS idx_t");
    static_assert(sizeof(real_t) == sizeof(double), "Requires 64-bit METIS real_t");

    int64_t nvtxs = mat.size();

    // set up metis parameters
    auto ncon = static_cast<int64_t>(mat.constraint_number());
    int64_t objval;
    std::vector<int64_t> options(METIS_NOPTIONS), part(nvtxs);
    std::vector<double> tpwgts(nparts * ncon, 1.0 / nparts), ubvec(ncon, imba_ratio);
    // set up weights vectorse

    std::vector<int64_t> node_wgts = scale_to_int64(mat.node_wgts()), edge_wgts = scale_to_int64(mat.edge_wgts());
    // check parameters

    /*std::cout << "nparts: " << nparts;
    std::cout << "nvtx: " << nvtxs;
    std::cout << "xadj: "
    summarize_vec(mat.xadj());
    std::cout << " adj: ";
    summarize_vec(mat.adj());
    std::cout << "ncon: " << ncon;*/

    // do partitioning
    METIS_SetDefaultOptions(options.data());
    METIS_PartGraphKway(&nvtxs,
                        &ncon,
                        to_int64(mat.xadj()).data(),
                        to_int64(mat.adj()).data(),
                        node_wgts.data(),
                        nullptr,
                        edge_wgts.data(),
                        &nparts,
                        tpwgts.data(),
                        ubvec.data(),
                        options.data(),
                        &objval,
                        part.data());
    return part;
}

#endif
