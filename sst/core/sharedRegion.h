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

#ifndef SST_CORE_CORE_SHAREDREGION_H
#define SST_CORE_CORE_SHAREDREGION_H

#include <sst/core/sst_types.h>

#include <string>


namespace SST {

class SharedRegion;


/**
 * Utility class to define how to merge multiple pieces of shared memory regions
 * Useful in the multi-MPI-rank, "Global Shared" model
 */
class SharedRegionMerger {
public:
    virtual ~SharedRegionMerger() { }
    /**
     * Merge the data from 'newData' into 'target'
     * @return True on success, False on failure
     */
    virtual bool operator()(void *target, size_t target_size, const void *newData, size_t newData_size) = 0;
};



class SharedRegionManager {
public:
    virtual SharedRegion* getLocalSharedRegion(const std::string &key, size_t size) = 0;
    virtual SharedRegion* getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger = NULL) = 0;

    virtual void publishRegion(SharedRegion*) = 0;
    virtual bool isRegionReady(const SharedRegion*) = 0;
    virtual void shutdownSharedRegion(SharedRegion*) = 0;

    virtual void updateState(bool finalize) = 0;
};



class SharedRegion {
private:
    SharedRegionManager *manager;
    size_t id;
    size_t size;
    void *memory;

protected:
    SharedRegion(SharedRegionManager *manager, size_t id,
            size_t size, void *memory) : manager(manager), id(id),
        size(size), memory(memory)
    { }

public:

    virtual ~SharedRegion()
    { }

    void shutdown() { manager->shutdownSharedRegion(this); }

    /**
     * @return The ID of this instance. (Number in range 0->N)
     */
    size_t getLocalShareID() const { return id; }

    /**
     * @return The size of the shared memory region
     */
    size_t getSize() const { return size; }

    /**
     * Call to denote that you are done making any changes to this region
     */
    void publish() { manager->publishRegion(this); }

    /**
     * @return True if the region is ready to use (all sharers have called publish()).
     */
    bool isReady() const { return manager->isRegionReady(this); }

    /**
     * @return a void* pointer to the shared memory region
     * This pointer is only valid to write to before a call to publish()
     */
    void* getRawPtr() { return memory; }

    /**
     * @return a const pointer to the shared memory region
     */
    template<typename T>
    T getPtr() const {
        return static_cast<T>(memory);
    }
};



}


#endif

