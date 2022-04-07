// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//#include <assert.h>

#include "sst_config.h"

#include "sst/core/testElements/coreTest_Serialization.h"

#include "sst/core/link.h"
#include "sst/core/objectSerialization.h"
#include "sst/core/rng/mersenne.h"
#include "sst/core/rng/rng.h"
#include "sst/core/warnmacros.h"

#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace SST {
namespace CoreTestSerialization {


template <typename T>
bool
checkSimpleSerializeDeserialize(T data)
{
    auto buffer = SST::Comms::serialize(data);
    T    result;
    SST::Comms::deserialize(buffer, result);
    return data == result;
};

template <typename T>
bool
checkContainerSerializeDeserialize(T& data)
{
    auto buffer = SST::Comms::serialize(data);
    T    result;
    SST::Comms::deserialize(buffer, result);

    if ( data.size() != result.size() ) return false;

    auto data_it   = data.begin();
    auto result_it = result.begin();

    // Only need to check one iterator since we already checked to
    // make sure they are equal size
    while ( data_it != data.end() ) {
        if ( *data_it != *result_it ) return false;
        ++data_it;
        ++result_it;
    }
    return true;
};

// For unordered containers
template <typename T>
bool
checkUContainerSerializeDeserialize(T& data)
{
    auto buffer = SST::Comms::serialize(data);
    T    result;
    SST::Comms::deserialize(buffer, result);

    if ( data.size() != result.size() ) return false;


    // Only need to check one iterator since we already checked to
    // make sure they are equal size
    for ( auto data_it = data.begin(); data_it != data.end(); ++data_it ) {
        bool match_found = false;
        for ( auto result_it = result.begin(); result_it != result.end(); ++result_it ) {
            if ( *data_it == *result_it ) {
                match_found = true;
                break;
            }
        }
        if ( !match_found ) return false;
    }
    return true;
};

coreTestSerialization::coreTestSerialization(ComponentId_t id, UNUSED(Params& params)) : Component(id)
{
    Output& out = getSimulationOutput();

    rng = new SST::RNG::MersenneRNG();
    // Test serialization for various data types

    // Simple Data Types
    // int8, int16, int32, int64, uint8, uint16, uint32, uint64, float, double, pair<int, int>, string
    bool passed;

    passed = checkSimpleSerializeDeserialize<int8_t>(rng->generateNextInt32());
    if ( !passed ) out.output("ERROR: int8_t did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<int16_t>(rng->generateNextInt32());
    if ( !passed ) out.output("ERROR: int16_t did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<int32_t>(rng->generateNextInt32());
    if ( !passed ) out.output("ERROR: int32_t did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<int64_t>(rng->generateNextInt64());
    if ( !passed ) out.output("ERROR: int64_t did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<uint8_t>(rng->generateNextUInt32());
    if ( !passed ) out.output("ERROR: uint8_t did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<uint16_t>(rng->generateNextUInt32());
    if ( !passed ) out.output("ERROR: uint16_t did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<uint32_t>(rng->generateNextUInt32());
    if ( !passed ) out.output("ERROR: uint32_t did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<uint64_t>(rng->generateNextUInt64());
    if ( !passed ) out.output("ERROR: uint64_t did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<float>(rng->nextUniform() * 1000);
    if ( !passed ) out.output("ERROR: float did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<double>(rng->nextUniform() * 1000000);
    if ( !passed ) out.output("ERROR: double did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize<std::string>("test string");
    if ( !passed ) out.output("ERROR: string did not serialize/deserialize properly\n");

    passed = checkSimpleSerializeDeserialize(
        std::make_pair<int32_t, int32_t>(rng->generateNextInt32(), rng->generateNextInt32()));
    if ( !passed ) out.output("ERROR: pair<int32_t,int32_t> did not serialize/deserialize properly\n");

    // Ordered Containers
    // map, set, vector, list, deque
    std::map<int32_t, int32_t> map_in;
    for ( int i = 0; i < 10; ++i )
        map_in[rng->generateNextInt32()] = rng->generateNextInt32();
    passed = checkContainerSerializeDeserialize(map_in);
    if ( !passed ) out.output("ERROR: map<int32_t,int32_t> did not serialize/deserialize properly\n");

    std::set<int32_t> set_in;
    for ( int i = 0; i < 10; ++i )
        set_in.insert(rng->generateNextInt32());
    passed = checkContainerSerializeDeserialize(set_in);
    if ( !passed ) out.output("ERROR: set<int32_t> did not serialize/deserialize properly\n");

    std::vector<int32_t> vector_in;
    for ( int i = 0; i < 10; ++i )
        vector_in.push_back(rng->generateNextInt32());
    passed = checkContainerSerializeDeserialize(vector_in);
    if ( !passed ) out.output("ERROR: vector<int32_t> did not serialize/deserialize properly\n");

    std::list<int32_t> list_in;
    for ( int i = 0; i < 10; ++i )
        list_in.push_back(rng->generateNextInt32());
    passed = checkContainerSerializeDeserialize(list_in);
    if ( !passed ) out.output("ERROR: list<int32_t> did not serialize/deserialize properly\n");

    std::deque<int32_t> deque_in;
    for ( int i = 0; i < 10; ++i )
        deque_in.push_back(rng->generateNextInt32());
    passed = checkContainerSerializeDeserialize(deque_in);
    if ( !passed ) out.output("ERROR: deque<int32_t> did not serialize/deserialize properly\n");

    // Unordered Containers
    // unordered_map, unordered_set
    std::unordered_map<int32_t, int32_t> umap_in;
    for ( int i = 0; i < 10; ++i )
        umap_in[rng->generateNextInt32()] = rng->generateNextInt32();
    passed = checkUContainerSerializeDeserialize(umap_in);
    if ( !passed ) out.output("ERROR: unordered_map<int32_t,int32_t> did not serialize/deserialize properly\n");

    std::unordered_set<int32_t> uset_in;
    for ( int i = 0; i < 10; ++i )
        uset_in.insert(rng->generateNextInt32());
    passed = checkUContainerSerializeDeserialize(uset_in);
    if ( !passed ) out.output("ERROR: unordered_set<int32_t,int32_t> did not serialize/deserialize properly\n");


    // Containers to other containers

    {
        // There is one instance where we serialize a
        // std::map<std::string, uintptr_t> and deserialize as a
        // std::vector<std::pair<std::string, uintptr_t>>, so check that
        // here
        std::map<std::string, uintptr_t> map2vec_in = {
            { "s1", 1 }, { "s2", 2 }, { "s3", 3 }, { "s4", 4 }, { "s5", 5 }
        };
        std::vector<std::pair<std::string, uintptr_t>> map2vec_out;

        auto buffer = SST::Comms::serialize(map2vec_in);
        SST::Comms::deserialize(buffer, map2vec_out);

        passed = true;
        // Check to see if we get the same data
        for ( auto& x : map2vec_out ) {
            if ( map2vec_in[x.first] != x.second ) passed = false;
        }
        if ( passed && map2vec_in.size() != map2vec_out.size() ) passed = false;
        if ( !passed )
            out.output("ERROR: serializing as map<string,uintptr_t> and deserializing to "
                       "vector<pair<string,uintptr_t>> did not work properly\n");
    }
}


} // namespace CoreTestSerialization
} // namespace SST
