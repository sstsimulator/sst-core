//leave translation unit empty if metis isn't provided

#include "sst/core/sst_config.h"

#ifdef HAVE_METIS
#include "sst/core/impl/partitioners/metispart.h"

#include "sst/core/warnmacros.h"
#include "sst/core/configGraph.h"

#include "sst/core/impl/partitioners/csrmat.hpp"

namespace SST {
namespace IMPL {
namespace Partition {

SSTMetisPartition::SSTMetisPartition(RankInfo world_size, RankInfo UNUSED(my_rank), int verbosity)
    : rankcount(world_size), partOutput(new Output("MetisPartition ", verbosity, 0, SST::Output::STDOUT))
{}

void SSTMetisPartition::performPartition(PartitionGraph* pgraph) {
    assert(rankcount.rank > 0);
    assert(partOutput);

    std::unordered_map<int, std::vector<double>> node_weights;
    std::unordered_map<std::pair<int,int>, double> edge_weights;
    std::unordered_map<int,int> component2group;

    //set node weights
    PartitionComponentMap_t& components = pgraph->getComponentMap();
    for ( auto iter = components.begin(); iter != components.end(); ++iter) {
        node_weights[iter->key()] = std::vector<double>(1,iter->weight);

        for ( auto& component_id : iter->group ) {
            component2group[component_id] = iter->key();
        }
    }

    //set edge weights
    PartitionLinkMap_t& links = pgraph->getLinkMap();
    for ( auto iter = links.begin(); iter != links.end(); ++iter ) {
        int group0 = component2group.at(iter->component[0]);
        int group1 = component2group.at(iter->component[1]);
        std::pair<int,int> key = std::make_pair(
            std::min(group0, group1), std::max(group0, group1));
        //give a uniform weight at the moment
        edge_weights[key] = 1;
    }

    try {
        partOutput->verbose(CALL_INFO, 1, 0,"Partitioning graph with %" PRIu64 " vertices\n",
                            node_weights.size());
        partOutput->verbose(CALL_INFO, 1, 0, "                    and %" PRIu64 " edges\n",
                            edge_weights.size());

        CSRMat csr(node_weights, edge_weights);

        //The imbalance ratio sets a goal imbalance for node weights across different
        //rank partitions
        constexpr double imbalance_ratio = 1.04;
        std::vector<int64_t> rank_partition = metis_part(csr,
                                                         rankcount.thread * rankcount.rank,
                                                         imbalance_ratio);

        std::vector<double> rank_weights(rankcount.thread * rankcount.rank, 0.);

        for ( uint i = 0; i < csr.size(); ++i ) {
            int64_t flat_rank = rank_partition[i];

            PartitionComponent& c = components[csr.get(i)];
            c.rank = RankInfo(flat_rank / rankcount.thread, flat_rank % rankcount.thread);

            rank_weights[flat_rank] += c.weight;
        }

        //print out partition quality
        double max_rank_weight{0.}, avg_rank_weight{0.};
        for ( auto w : rank_weights ) {
            max_rank_weight = std::max(w, max_rank_weight);
            avg_rank_weight += w;
        }
        avg_rank_weight /= (rankcount.thread * rankcount.rank);

        partOutput->verbose(CALL_INFO, 1, 0, "Partition imbalance (max/avg rank weight): %f\n",
                            max_rank_weight/avg_rank_weight);

        double sum_edge_weights{0.}, cut_weights{0.};
        for ( auto& ew : edge_weights ) {
            auto it0 = std::lower_bound(csr.node_ID().cbegin(), csr.node_ID().cend(),
                                        ew.first.first);
            auto it1 = std::lower_bound(it0, csr.node_ID().cend(), ew.first.second);

            sum_edge_weights += ew.second;
            if ( rank_partition[*it0] != rank_partition[*it1] ) {
                cut_weights += ew.second;
            }
        }
        double cut_pct = cut_weights/sum_edge_weights*100;
        partOutput->verbose(CALL_INFO, 1, 0, "Percentage of edges cut: %f\n",
                            cut_pct);

    } catch (const std::exception& e) {
        partOutput->fatal(CALL_INFO, 1, 0, e.what());
    }

    partOutput->verbose(CALL_INFO, 1, 0, "Metis partitioner finished.\n");
}
}
}
}

#endif
