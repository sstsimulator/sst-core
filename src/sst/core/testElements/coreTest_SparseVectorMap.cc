// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/testElements/coreTest_SparseVectorMap.h"

#include "sst/core/output.h"
#include "sst/core/sparseVectorMap.h"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace SST::CoreTestSparseVectorMap {

namespace {

//////////////////////////////////////////////////////////////////////
// SST::SparseVectorMap.  Serialization is tested by the
// coreTestSerialization component.
//////////////////////////////////////////////////////////////////////

using SST::SparseVectorMap;

struct Record
{
    uint64_t id_;
    uint64_t payload;
    uint64_t key() const { return id_; }
};

struct NoKeyRecord
{
    uint64_t recordId;
    uint64_t payload;
};

struct NoKeyExtract
{
    static uint64_t get(const NoKeyRecord& r) { return r.recordId; }
};

// A permutation of 0..79, out of order and comfortably past
// SparseVectorMap's 64-element no-tail threshold, so maps built from
// these keys (inserted one at a time, via makeRecordMap() below) hold
// a genuine unmerged tail.  Below that threshold every insert goes
// directly to its sorted position and no tail is ever created -- see
// sparseVectorMap.h.
std::vector<uint64_t>
makeSvmKeys()
{
    constexpr uint64_t    n = 80;
    std::vector<uint64_t> keys;
    keys.reserve(n);
    for ( uint64_t i = 0; i < n; ++i )
        keys.push_back((i * 7919) % n); // 7919 is coprime with n, so this is a permutation
    return keys;
}
const std::vector<uint64_t> svmKeys = makeSvmKeys();

uint64_t
recordKey(const Record& r)
{
    return r.id_;
}

SparseVectorMap<uint64_t, Record>
makeRecordMap()
{
    SparseVectorMap<uint64_t, Record> map;
    for ( uint64_t k : svmKeys )
        map.insert(Record { k, k * 10 });
    return map;
}

// True if iterating map yields strictly increasing keys and exactly
// expectedCount elements.
template <typename MapT, typename KeyFn>
bool
iteratesSorted(MapT& map, size_t expectedCount, KeyFn keyOf)
{
    uint64_t prev  = 0;
    bool     first = true;
    size_t   count = 0;
    for ( auto it = map.begin(); it != map.end(); ++it ) {
        uint64_t k = keyOf(*it);
        if ( !first && !(prev < k) ) return false;
        prev  = k;
        first = false;
        ++count;
    }
    return count == expectedCount;
}

// insert(), contains() and operator[] with an unmerged tail
void
testSvmLookup(Output& out)
{
    SparseVectorMap<uint64_t, Record> empty;
    if ( !empty.empty() || empty.size() != 0 ) out.output("ERROR: SparseVectorMap new map is not empty\n");

    auto map = makeRecordMap();
    if ( map.empty() || map.size() != svmKeys.size() )
        out.output("ERROR: SparseVectorMap size incorrect after insert\n");
    for ( uint64_t k : svmKeys ) {
        if ( !map.contains(k) || map[k].payload != k * 10 )
            out.output("ERROR: SparseVectorMap lookup incorrect for key %llu\n", (unsigned long long)k);
    }
    if ( map.contains(100) ) out.output("ERROR: SparseVectorMap contains() found an absent key\n");

    Record& existing = map.insert(Record { 4, 999 });
    if ( existing.payload != 40 || map.size() != svmKeys.size() )
        out.output("ERROR: SparseVectorMap duplicate insert was not a no-op\n");

    bool threw = false;
    try {
        (void)map[100];
    }
    catch ( const std::out_of_range& ) {
        threw = true;
    }
    if ( !threw ) out.output("ERROR: SparseVectorMap operator[] on an absent key did not throw\n");

    const auto& cmap = map;
    if ( !cmap.contains(4) || cmap[4].payload != 40 ) out.output("ERROR: SparseVectorMap const lookup incorrect\n");
}

// Reads never move elements, so references stay valid until the next write
void
testSvmReferenceStability(Output& out)
{
    SparseVectorMap<uint64_t, Record> map;
    for ( uint64_t k = 100; k < 200; ++k )
        map.insert(Record { k, k * 10 });
    map.insert(Record { 10, 100 });
    map.insert(Record { 50, 500 });
    Record& inserted     = map.insert(Record { 30, 300 }); // last write: lands in the tail
    Record* insertedAddr = &inserted;
    Record* prefixAddr   = &map[150];
    Record* tailAddr     = &map[10];

    for ( int rep = 0; rep < 10; ++rep ) {
        for ( uint64_t k = 0; k < 250; ++k ) {
            if ( map.contains(k) ) (void)map[k].payload;
        }
    }
    if ( &map[30] != insertedAddr || inserted.payload != 300 )
        out.output("ERROR: SparseVectorMap reads invalidated a reference returned by insert()\n");
    if ( &map[150] != prefixAddr || &map[10] != tailAddr )
        out.output("ERROR: SparseVectorMap reads invalidated a reference returned by operator[]\n");
}

// find(), erase() and clear()
void
testSvmFindEraseClear(Output& out)
{
    auto map = makeRecordMap();

    auto it = map.find(3);
    if ( it == map.end() || it->payload != 30 ) out.output("ERROR: SparseVectorMap find() of a present key failed\n");
    if ( map.find(100) != map.end() )
        out.output("ERROR: SparseVectorMap find() of an absent key did not return end()\n");
    const auto& cmap = map;
    if ( cmap.find(3) == cmap.end() || cmap.find(100) != cmap.end() )
        out.output("ERROR: SparseVectorMap const find() incorrect\n");

    if ( map.erase(4) != 1 ) out.output("ERROR: SparseVectorMap erase() of a present key failed\n");
    if ( map.erase(4) != 0 || map.erase(100) != 0 )
        out.output("ERROR: SparseVectorMap erase() of an absent key did not return 0\n");
    if ( map.contains(4) || map.size() != svmKeys.size() - 1 )
        out.output("ERROR: SparseVectorMap state incorrect after erase()\n");

    // Key 4 is below the current maximum, so it goes into the tail
    map.insert(Record { 4, 40 });
    if ( map.erase(4) != 1 || map.contains(4) || map.size() != svmKeys.size() - 1 )
        out.output("ERROR: SparseVectorMap erase() from the tail failed\n");

    map.clear();
    if ( !map.empty() || map.size() != 0 ) out.output("ERROR: SparseVectorMap not empty after clear()\n");
    map.insert(Record { 42, 420 });
    if ( !map.contains(42) || map.size() != 1 ) out.output("ERROR: SparseVectorMap unusable after clear()\n");
}

// Sorted iteration with a pending tail
void
testSvmIteration(Output& out)
{
    auto map = makeRecordMap();
    if ( !iteratesSorted(map, svmKeys.size(), recordKey) )
        out.output("ERROR: SparseVectorMap iteration not sorted with a pending tail\n");

    size_t count = 0;
    for ( auto& r : map ) {
        if ( r.payload != r.id_ * 10 ) out.output("ERROR: SparseVectorMap iteration returned a wrong element\n");
        ++count;
    }
    if ( count != svmKeys.size() ) out.output("ERROR: SparseVectorMap range-for visited the wrong count\n");
}

// Constructing from a vector, sorted and unsorted
void
testSvmVectorConstructor(Output& out)
{
    std::vector<Record> unsorted;
    for ( uint64_t k : svmKeys )
        unsorted.push_back(Record { k, k * 10 });
    SparseVectorMap<uint64_t, Record> fromUnsorted(unsorted, false);
    if ( !unsorted.empty() ) out.output("ERROR: SparseVectorMap vector constructor did not take the data\n");
    if ( !iteratesSorted(fromUnsorted, svmKeys.size(), recordKey) || !fromUnsorted.contains(8) )
        out.output("ERROR: SparseVectorMap vector constructor (unsorted) incorrect\n");

    std::vector<Record> presorted;
    for ( uint64_t k = 0; k < 9; ++k )
        presorted.push_back(Record { k, k * 10 });
    SparseVectorMap<uint64_t, Record> fromSorted(presorted, true);
    if ( !iteratesSorted(fromSorted, 9, recordKey) || !fromSorted.contains(0) )
        out.output("ERROR: SparseVectorMap vector constructor (sorted) incorrect\n");
}

// Pointer elements and filter()
void
testSvmPointerFilter(Output& out)
{
    SparseVectorMap<uint64_t, Record*> map;
    for ( uint64_t k : svmKeys )
        map.insert(new Record { k, k * 10 });
    for ( uint64_t k : svmKeys ) {
        if ( !map.contains(k) || map[k]->payload != k * 10 )
            out.output("ERROR: SparseVectorMap<pointer> lookup incorrect for key %llu\n", (unsigned long long)k);
    }

    // Drop even keys; the filter owns disposal of what it drops
    struct DropEven
    {
        Record* operator()(Record* r)
        {
            if ( r->id_ % 2 == 0 ) {
                delete r;
                return nullptr;
            }
            return r;
        }
    } dropEven;
    map.filter(dropEven);

    size_t odd = 0;
    for ( uint64_t k : svmKeys )
        if ( k % 2 == 1 ) ++odd;
    if ( map.size() != odd ) out.output("ERROR: SparseVectorMap<pointer> filter() kept the wrong count\n");
    if ( !iteratesSorted(map, odd, [](const Record* r) { return r->id_; }) )
        out.output("ERROR: SparseVectorMap<pointer> not sorted after filter()\n");
    for ( auto* r : map ) {
        if ( r->id_ % 2 != 1 ) out.output("ERROR: SparseVectorMap<pointer> filter() kept an even key\n");
        delete r;
    }
}

// Set-like usage (keyT == classT)
void
testSvmSet(Output& out)
{
    SparseVectorMap<uint64_t> map;
    for ( uint64_t k : svmKeys )
        map.insert(k);
    map.insert(4);
    if ( map.size() != svmKeys.size() ) out.output("ERROR: SparseVectorMap<set> size incorrect\n");
    for ( uint64_t k : svmKeys ) {
        if ( !map.contains(k) )
            out.output("ERROR: SparseVectorMap<set> contains() incorrect for key %llu\n", (unsigned long long)k);
    }
    if ( !iteratesSorted(map, svmKeys.size(), [](uint64_t k) { return k; }) )
        out.output("ERROR: SparseVectorMap<set> iteration not sorted\n");
}

// Custom KeyExtract on a type with no key() method
void
testSvmKeyExtract(Output& out)
{
    SparseVectorMap<uint64_t, NoKeyRecord, NoKeyExtract> map;
    for ( uint64_t k : svmKeys )
        map.insert(NoKeyRecord { k, k * 10 });
    for ( uint64_t k : svmKeys ) {
        if ( !map.contains(k) || map[k].payload != k * 10 )
            out.output(
                "ERROR: SparseVectorMap<custom KeyExtract> lookup incorrect for key %llu\n", (unsigned long long)k);
    }
    if ( !iteratesSorted(map, svmKeys.size(), [](const NoKeyRecord& r) { return r.recordId; }) )
        out.output("ERROR: SparseVectorMap<custom KeyExtract> iteration not sorted\n");
}

// std::pair<keyT, V> elements, keyed by first with the default KeyOf.
// Accessors return the whole pair.
void
testSvmPair(Output& out)
{
    using Entry = std::pair<uint64_t, std::string>;
    SparseVectorMap<uint64_t, Entry> map;
    for ( uint64_t k : svmKeys ) {
        Entry& e = map.insert(Entry { k, std::to_string(k * 10) });
        if ( e.first != k || e.second != std::to_string(k * 10) )
            out.output("ERROR: SparseVectorMap<pair> insert() returned the wrong element\n");
    }
    if ( map.size() != svmKeys.size() ) out.output("ERROR: SparseVectorMap<pair> size incorrect\n");
    for ( uint64_t k : svmKeys ) {
        if ( !map.contains(k) || map[k].first != k || map[k].second != std::to_string(k * 10) )
            out.output("ERROR: SparseVectorMap<pair> lookup incorrect for key %llu\n", (unsigned long long)k);
    }
    if ( map.contains(1000) ) out.output("ERROR: SparseVectorMap<pair> contains() found an absent key\n");

    if ( map.insert(Entry { 4, "other" }).second != "40" || map.size() != svmKeys.size() )
        out.output("ERROR: SparseVectorMap<pair> duplicate insert was not a no-op\n");
    map[4].second = "changed";
    if ( map[4].second != "changed" ) out.output("ERROR: SparseVectorMap<pair> could not modify second\n");

    const auto& cmap = map;
    auto        it   = cmap.find(8);
    if ( it == cmap.end() || it->first != 8 || it->second != "80" )
        out.output("ERROR: SparseVectorMap<pair> const find() incorrect\n");
    if ( !iteratesSorted(map, svmKeys.size(), [](const Entry& e) { return e.first; }) )
        out.output("ERROR: SparseVectorMap<pair> iteration not sorted\n");

    if ( map.erase(4) != 1 || map.contains(4) ) out.output("ERROR: SparseVectorMap<pair> erase() failed\n");

    // Methods that work on second
    if ( map.value(8) != "80" || cmap.value(8) != "80" ) out.output("ERROR: SparseVectorMap<pair> value() incorrect\n");
    bool threw = false;
    try {
        (void)map.value(4);
    }
    catch ( const std::out_of_range& ) {
        threw = true;
    }
    if ( !threw ) out.output("ERROR: SparseVectorMap<pair> value() on an absent key did not throw\n");

    // at() behaves like std::map::at()
    if ( map.at(8) != "80" || &map.at(8) != &map.value(8) || &cmap.at(8) != &map[8].second )
        out.output("ERROR: SparseVectorMap<pair> at() incorrect\n");
    map.at(8) = "changed";
    if ( map[8].second != "changed" ) out.output("ERROR: SparseVectorMap<pair> could not modify through at()\n");
    map.at(8) = "80";
    threw     = false;
    try {
        (void)cmap.at(4);
    }
    catch ( const std::out_of_range& ) {
        threw = true;
    }
    if ( !threw ) out.output("ERROR: SparseVectorMap<pair> at() on an absent key did not throw\n");
    const std::string* found = cmap.findValue(8);
    if ( found == nullptr || *found != "80" || found != &map[8].second )
        out.output("ERROR: SparseVectorMap<pair> findValue() of a present key incorrect\n");
    if ( map.findValue(4) != nullptr || cmap.findValue(1000) != nullptr )
        out.output("ERROR: SparseVectorMap<pair> findValue() of an absent key did not return nullptr\n");

    std::string& assigned = map.insert_or_assign(8, "eighty");
    if ( assigned != "eighty" || map[8].second != "eighty" || &assigned != found || map.size() != svmKeys.size() - 1 )
        out.output("ERROR: SparseVectorMap<pair> insert_or_assign() did not replace the value in place\n");
    if ( map.insert_or_assign(4, "forty") != "forty" || map.value(4) != "forty" || map.size() != svmKeys.size() )
        out.output("ERROR: SparseVectorMap<pair> insert_or_assign() did not insert\n");
    if ( map.insert(4, "other") != "forty" || map.size() != svmKeys.size() )
        out.output("ERROR: SparseVectorMap<pair> two-argument insert() replaced an existing value\n");
    if ( map.insert(1000, "1000") != "1000" || !map.contains(1000) || map.size() != svmKeys.size() + 1 )
        out.output("ERROR: SparseVectorMap<pair> two-argument insert() did not insert\n");
}

// Many out-of-order inserts interleaved with reads, compared against
// std::map.  Enough keys to cross the tail size limit many times.
void
testSvmInterleaved(Output& out)
{
    const uint64_t                    n = 5000;
    SparseVectorMap<uint64_t, Record> map;
    std::map<uint64_t, uint64_t>      oracle;
    bool                              ok = true;
    for ( uint64_t i = 0; i < n && ok; ++i ) {
        uint64_t k = (i * 7919) % n; // 7919 is coprime with n, so this is a permutation
        map.insert(Record { k, k * 10 });
        oracle[k] = k * 10;
        if ( !map.contains(k) || map[k].payload != k * 10 ) ok = false;
        uint64_t probe = (i * 31) % n;
        if ( map.contains(probe) != (oracle.count(probe) != 0) ) ok = false;
    }
    if ( !ok ) out.output("ERROR: SparseVectorMap interleaved out-of-order insert/read incorrect\n");
    if ( map.size() != oracle.size() ) out.output("ERROR: SparseVectorMap interleaved final size incorrect\n");
    for ( auto& kv : oracle ) {
        if ( !map.contains(kv.first) || map[kv.first].payload != kv.second ) {
            out.output("ERROR: SparseVectorMap interleaved final contents incorrect\n");
            break;
        }
    }
    if ( !iteratesSorted(map, oracle.size(), recordKey) )
        out.output("ERROR: SparseVectorMap interleaved iteration not sorted\n");
}

// True if every key in keys is present with payload == key * 10 and
// const begin()/end()/find() leave every element where operator[]
// found it, i.e. there was nothing left to merge.
bool
compactedAndIntact(const SparseVectorMap<uint64_t, Record>& map, const std::vector<uint64_t>& keys)
{
    std::map<uint64_t, const Record*> addr;
    for ( uint64_t k : keys ) {
        if ( !map.contains(k) || map[k].payload != k * 10 ) return false;
        addr[k] = &map[k];
    }
    (void)map.begin();
    (void)map.end();
    for ( uint64_t k : keys ) {
        auto it = map.find(k);
        if ( it == map.end() || &*it != addr[k] || &map[k] != addr[k] ) return false;
    }
    return true;
}

// compact(), with and without releasing memory, and inserting afterwards
void
testSvmCompact(Output& out)
{
    {
        auto map = makeRecordMap(); // holds a pending tail
        map.compact();
        if ( map.size() != svmKeys.size() || !compactedAndIntact(map, svmKeys) )
            out.output("ERROR: SparseVectorMap compact() did not leave a fully merged map\n");
        if ( !iteratesSorted(map, svmKeys.size(), recordKey) )
            out.output("ERROR: SparseVectorMap not sorted after compact()\n");

        // Inserting after compact() is allowed: out of order (40 is
        // below the maximum, so it opens a new tail) and in order
        map.erase(40);
        map.insert(Record { 40, 400 });
        map.insert(Record { 1000, 10000 });
        if ( map.size() != svmKeys.size() + 1 || !map.contains(40) || !map.contains(1000) )
            out.output("ERROR: SparseVectorMap insert() after compact() incorrect\n");
        if ( !iteratesSorted(map, svmKeys.size() + 1, recordKey) )
            out.output("ERROR: SparseVectorMap not sorted after insert() following compact()\n");
        map.compact();
        std::vector<uint64_t> all = svmKeys;
        all.push_back(1000);
        if ( !compactedAndIntact(map, all) )
            out.output("ERROR: SparseVectorMap second compact() did not leave a fully merged map\n");
    }
    {
        auto map = makeRecordMap();
        map.compact(false);
        if ( map.size() != svmKeys.size() || !compactedAndIntact(map, svmKeys) )
            out.output("ERROR: SparseVectorMap compact(false) did not leave a fully merged map\n");
    }
    {
        SparseVectorMap<uint64_t, Record> empty;
        empty.compact();
        if ( !empty.empty() ) out.output("ERROR: SparseVectorMap compact() of an empty map is not empty\n");
        empty.insert(Record { 7, 70 });
        if ( !empty.contains(7) ) out.output("ERROR: SparseVectorMap unusable after compact() of an empty map\n");
    }
}

using TestFn = void (*)(Output&);

// Test name (the component's "test" parameter) -> test function
const std::map<std::string, TestFn>&
sparseVectorMapTests()
{
    static const std::map<std::string, TestFn> tests = {
        { "sparsevectormap_lookup", testSvmLookup },
        { "sparsevectormap_reference_stability", testSvmReferenceStability },
        { "sparsevectormap_find_erase_clear", testSvmFindEraseClear },
        { "sparsevectormap_iteration", testSvmIteration },
        { "sparsevectormap_vector_constructor", testSvmVectorConstructor },
        { "sparsevectormap_pointer_filter", testSvmPointerFilter },
        { "sparsevectormap_set", testSvmSet },
        { "sparsevectormap_keyextract", testSvmKeyExtract },
        { "sparsevectormap_interleaved", testSvmInterleaved },
        { "sparsevectormap_compact", testSvmCompact },
        { "sparsevectormap_pair", testSvmPair },
    };
    return tests;
}

} // namespace

coreTestSparseVectorMap::coreTestSparseVectorMap(ComponentId_t id, Params& params) :
    Component(id)
{
    Output& out = getSimulationOutput();

    std::string test = params.find<std::string>("test");
    if ( test == "" ) out.fatal(CALL_INFO_LONG, 1, "ERROR: Must specify test type\n");

    auto it = sparseVectorMapTests().find(test);
    if ( it == sparseVectorMapTests().end() )
        out.fatal(CALL_INFO_LONG, 1, "ERROR: Unknown SparseVectorMap test specified: %s\n", test.c_str());
    it->second(out);
}

} // namespace SST::CoreTestSparseVectorMap
