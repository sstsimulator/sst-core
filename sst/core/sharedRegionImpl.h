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

#ifndef SST_CORE_CORE_SHAREDREGIONIMPL_H
#define SST_CORE_CORE_SHAREDREGIONIMPL_H

#include <sst/core/sst_types.h>

#include <string>
#include <vector>
#include <set>
#include <map>

#include <sst/core/sharedRegion.h>

namespace SST {

class SharedRegionImpl;

class RegionInfo {
    std::string myKey;
    size_t realSize;
    size_t apparentSize;
    void *memory;

    size_t shareCount;
    size_t publishCount;

    std::vector<SharedRegionImpl*> sharers;

    std::set<int> ranks;
    SharedRegionMerger *merger; // If null, no multi-rank merging
    bool initialized;
    bool ready;

public:
    RegionInfo() : realSize(0), apparentSize(0), memory(NULL),
        shareCount(0), publishCount(0), merger(NULL),
        initialized(false), ready(false)
    { }
    ~RegionInfo();
    bool initialize(const std::string &key, size_t size, SharedRegionMerger *mergeObj);
    bool isInitialized() const { return initialized; }
    bool isReady() const { return ready; }
    SharedRegionImpl* addSharer(SharedRegionManager *manager);
    void removeSharer(SharedRegionImpl *sri);
    void publish();
    void updateState(bool finalize);
    const std::string& getKey() const { return myKey; }
    void* getMemory() const { return memory; }
    size_t getSize() const { return apparentSize; }
    size_t getNumSharers() const { return shareCount; }
};


class SharedRegionImpl : public SharedRegion {
    bool published;
    RegionInfo *region;
public:
    SharedRegionImpl(SharedRegionManager *manager, size_t id,
            size_t size, RegionInfo *region) : SharedRegion(manager, id,
                size, region->getMemory()), published(false), region(region)
    { }

    bool isPublished() const { return published; }
    void setPublished() { published = true; }
    RegionInfo* getRegion() const { return region; }
};



class SharedRegionManagerImpl : public SharedRegionManager {

    std::map<std::string, RegionInfo> regions;

public:
    SharedRegionManagerImpl();
    ~SharedRegionManagerImpl();

    virtual SharedRegion* getLocalSharedRegion(const std::string &key, size_t size);
    virtual SharedRegion* getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger = NULL);

    virtual void publishRegion(SharedRegion*);
    virtual bool isRegionReady(const SharedRegion*);
    virtual void shutdownSharedRegion(SharedRegion*);

    void updateState(bool finalize);
};


}
#endif
