// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//
#include "sst_config.h"

#include "sst/core/interfaces/simpleNetwork.h"

#include "sst/core/objectComms.h"

using namespace std;

namespace SST {
namespace Interfaces {

const SimpleNetwork::nid_t SimpleNetwork::INIT_BROADCAST_ADDR = 0xffffffffffffffffl;

map<string, vector<SimpleNetwork::nid_t> > SimpleNetwork::network_maps;

SimpleNetwork::nid_t SimpleNetwork::Mapping::operator[](SimpleNetwork::nid_t from) const {
    if ( from > (SimpleNetwork::nid_t)data->size() ) {
        // fatal
    }
    return (*data)[from];
}

void SimpleNetwork::Mapping::bind(string name){
    data = getMappingVector(name);
}

void SimpleNetwork::addMappingEntry(string mapName, nid_t from, nid_t to) {
    if ( network_maps.find(mapName) == network_maps.end() ) {
        network_maps.insert(std::make_pair(mapName, vector<SimpleNetwork::nid_t>()));
    }
    
    vector<SimpleNetwork::nid_t>& map = network_maps[mapName];
    if ( (SimpleNetwork::nid_t)map.size() < (from + 1) ) {
        map.resize(from + 1, SimpleNetwork::INIT_BROADCAST_ADDR);
    }
    if ( map[from] == SimpleNetwork::INIT_BROADCAST_ADDR ) {
        map[from] = to;
    }
    else {
        if (map[from] != to) {
            // fatal
        }
    }
}

vector<SimpleNetwork::nid_t>* SimpleNetwork::getMappingVector(string mapName) {
    if ( network_maps.find(mapName) == network_maps.end() ) {
        // fatal
    }
    return &network_maps[mapName];
}

void SST::Interfaces::SimpleNetwork::exchangeMappingData() {
#if SST_CONFIG_HAVE_MPI
    int rank = Simulation::getSimulation()->getRank().rank;
    int num_ranks = Simulation::getSimulation()->getNumRanks().rank;
    if ( num_ranks > 1 ) {

        // Send all mappings to next lowest rank
        if ( rank == num_ranks - 1 ) {
            SST::Comms::send(rank  - 1, 0, network_maps);
            network_maps.clear();
        }
        else {
            map<string,vector<SimpleNetwork::nid_t> > maps;
            SST::Comms::recv(rank + 1, 0, maps);

            // Now loop through all the Mapping objects and combine
            // them, then send to next lower rank. Easiest to just put
            // mine into the one I just received then send it on.
            for ( auto my_iter = network_maps.begin();
                  my_iter != network_maps.end(); ++my_iter ) {
                string name = my_iter->first;
                vector<SimpleNetwork::nid_t>& my_vec = my_iter->second;

                if ( maps.find(name) == maps.end() ) {
                    // If it's not there, just copy it over
                    maps.insert(std::make_pair(name, my_vec));
                }
                else {
                    auto iter = maps.find(name);
                    vector<SimpleNetwork::nid_t>& vec = iter->second;
                    // If it is there, need to combine vectors and
                    // check for duplicates that don't match.
                    if ( vec.size() < my_vec.size() ) {
                        vec.resize(my_vec.size(), SimpleNetwork::INIT_BROADCAST_ADDR);
                    }

                    size_t num_to_copy = vec.size() < my_vec.size() ? vec.size() : my_vec.size();
                    
                    for ( size_t i = 0; i < num_to_copy; i++ ) {
                        if ( vec[i] == SimpleNetwork::INIT_BROADCAST_ADDR ) {
                            vec[i] = my_vec[i];
                        }
                        else if ( my_vec[i] == SimpleNetwork::INIT_BROADCAST_ADDR ) {
                            // Do nothing, no need to copy
                        }
                        else if ( my_vec[i] != vec[i] ) {
                            // fatal, they don't match
                        }
                    }
                }
            }
            
            // Send it on to next rank down
            if ( rank != 0 ) {
                SST::Comms::send(rank - 1, 0, maps);
                network_maps.clear();
            }
            else {
                network_maps = maps;
                maps.clear();
            }

        }
        SST::Comms::broadcast(network_maps, 0);

    }
#endif
}

} // namespace Interfaces
} // namespace SST

