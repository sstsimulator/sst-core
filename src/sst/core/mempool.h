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

#ifndef SST_CORE_MEMPOOL_H
#define SST_CORE_MEMPOOL_H

#include "sst/core/serialization/serializable.h"

namespace SST {
class Output;

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
    static void getMemPoolUsage(uint64_t& bytes, uint64_t& active_entries);

    // Initialize the global mempool data structures
    static void initializeGlobalData(int num_threads);

    // Initialize the per thread mempool ata structures
    static void initializeLocalData(int thread);
};

// Base class for those classes that will use mempools.  Mempools are
// designed to be used primarily with Activities/Events and small data
// strcutures that are part of events.  Thus, MemPoolItem inherits
// from Serializable because all these classes will generally need to
// serialize.
class MemPoolItem : public SST::Core::Serialization::serializable
{
protected:
    MemPoolItem() {}

public:
    /** Allocates memory from a memory pool for a new Activity */
    void* operator new(std::size_t size) noexcept;

    /** Returns memory for this Activity to the appropriate memory pool */
    void operator delete(void* ptr);

    /** Get a string represenation of the entry.  The default version
     * will just use the name of the class, retrieved through the
     * cls_name() function inherited from the serialzable class, which
     * will return the name of the last class to call one of the
     * serialization macros (ImplementSerializable(),
     * ImplementVirtualSerializable(), or NotSerializable()).
     * Subclasses can override this function if they want to add
     * additional information.
     */
    virtual std::string toString() const;


    virtual void print(const std::string& header, Output& out) const;

    ImplementVirtualSerializable(SST::Core::MemPoolItem)
};

} // namespace Core
} // namespace SST

#endif // SST_CORE_MEMPOOL_H
