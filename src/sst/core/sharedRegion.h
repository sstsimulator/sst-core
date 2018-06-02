// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_SHAREDREGION_H
#define SST_CORE_CORE_SHAREDREGION_H

#include <sst/core/sst_types.h>

#include <string>
#include <vector>


namespace SST {

class SharedRegion;
class ChangeSet;

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
    virtual bool merge(uint8_t *target, const uint8_t *newData, size_t size);
    virtual bool merge(uint8_t *target, size_t size, const std::vector<ChangeSet> &changeSets);
};


class SharedRegionInitializedMerger : public SharedRegionMerger {
    uint8_t defVal;
public:
    SharedRegionInitializedMerger(uint8_t defaultValue) {
        defVal = defaultValue;
    }

    bool merge(uint8_t *target, const uint8_t *newData, size_t size) override;

};


class SharedRegionManager {
protected:
    friend class SharedRegion;

    virtual void modifyRegion(SharedRegion *sr, size_t offset, size_t length, const void *data) = 0;
    virtual void* getMemory(SharedRegion* sr) = 0;
    virtual const void* getConstPtr(const SharedRegion* sr) const = 0;

public:
    virtual SharedRegion* getLocalSharedRegion(const std::string &key, size_t size, uint8_t initByte = 0) = 0;
    virtual SharedRegion* getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger = NULL, uint8_t initByte = 0) = 0;

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

protected:
    SharedRegion(SharedRegionManager *manager, size_t id,
            size_t size) : manager(manager), id(id),
        size(size)
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
     * Before the region has published, apply a modification. (Copy this data in)
     */
    void modifyRegion(size_t offset, size_t length, const void *data)
    { manager->modifyRegion(this, offset, length, data); }

    template<typename T>
    void modifyArray(size_t offset, const T &data)
    { manager->modifyRegion(this, offset*sizeof(T), sizeof(T), &data); }

    /**
     * @return a void* pointer to the shared memory region
     * This pointer is only valid to write to before a call to publish()
     */
    void* getRawPtr() { return manager->getMemory(this); }

    /**
     * @return a const pointer to the shared memory region
     */
    template<typename T>
    T getPtr() const {
        return static_cast<T>(manager->getConstPtr(this));
    }
};



}


#endif

