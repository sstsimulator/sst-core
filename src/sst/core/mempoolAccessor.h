// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MEMPOOL_ACCESSOR_H
#define SST_CORE_MEMPOOL_ACCESSOR_H


namespace SST {

namespace Core {

// Class to access stats/data about the mempools.  This is here to
// limit exposure to the USE_MEMPOOL #define, which will only be in
// core .cc files.
class MemPoolAccessor
{
public:
    // Gets the arena size for the specified pool size on the current
    // thread.  If mempools aren't enabled, it will return 0.
    static size_t getArenaSize(size_t size);

    // Gets the number of arenas allocated for the specified pool size
    // on the current thread.  If mempools aren't enabled, it will
    // return 0.
    static size_t getNumArenas(size_t size);

    // Gets the total bytes used for the specified pool size on the
    // current thread.  If mempools aren't enabled, it will return 0.
    static uint64_t getBytesMemUsedBy(size_t size);

    // Gets the total mempool usage for the rank.  Returns both the
    // bytes and the number of active entries.  Bytes and entries are
    // added to the value passed into the function.  If mempools
    // aren't enabled, then nothing will be counted.
    static void getMemPoolUsage(int64_t& bytes, int64_t& active_entries);

    // Initialize the global mempool data structures
    static void initializeGlobalData(int num_threads, bool cache_align = false);

    // Initialize the per thread mempool ata structures
    static void initializeLocalData(int thread);

    static void printUndeletedMemPoolItems(const std::string& header, Output& out);
};

} // namespace Core
} // namespace SST

#endif // SST_CORE_MEMPOOL_ACCESSOR_H
