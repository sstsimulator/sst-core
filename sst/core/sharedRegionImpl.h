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
public:
    class RegionMergeInfo {

    protected:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version );

        RegionMergeInfo() {}
        int rank;
        std::string key;

    public:
        RegionMergeInfo(int rank, const std::string &key) : rank(rank), key(key) { }
        virtual ~RegionMergeInfo() { }

        virtual bool merge(RegionInfo *ri) { return true; }
        const std::string& getKey() const { return key; }
    };


    class BulkMergeInfo : public RegionMergeInfo {
    protected:
        BulkMergeInfo() : RegionMergeInfo() {}
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version );

        size_t length;
        void *data;

    public:
        BulkMergeInfo(int rank, const std::string &key, void *data, size_t length) : RegionMergeInfo(rank, key),
            length(length), data(data)
        { }

        bool merge(RegionInfo *ri) {
            bool ret = ri->getMerger()->merge((uint8_t*)ri->getMemory(), (const uint8_t*)data, length);
            free(data);
            return ret;
        }
    };

    class ChangeSetMergeInfo : public RegionMergeInfo {
    protected:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version );

        ChangeSetMergeInfo() : RegionMergeInfo() {}

        std::vector<SharedRegionMerger::ChangeSet> changeSets;
    public:
        ChangeSetMergeInfo(int rank, const std::string & key,
                std::vector<SharedRegionMerger::ChangeSet> & changeSets) : RegionMergeInfo(rank, key),
            changeSets(changeSets)
        { }
        bool merge(RegionInfo *ri) {
            return ri->getMerger()->merge((uint8_t*)ri->getMemory(), ri->getSize(), changeSets);
        }
    };


private:
    std::string myKey;
    size_t realSize;
    size_t apparentSize;
    void *memory;

    size_t shareCount;
    size_t publishCount;

    std::vector<SharedRegionImpl*> sharers;

    SharedRegionMerger *merger; // If null, no multi-rank merging
    std::vector<SharedRegionMerger::ChangeSet> changesets;

    bool didBulk;
    bool initialized;
    bool ready;


public:
    RegionInfo() : realSize(0), apparentSize(0), memory(NULL),
        shareCount(0), publishCount(0), merger(NULL),
        didBulk(false), initialized(false), ready(false)
    { }
    ~RegionInfo();
    bool initialize(const std::string &key, size_t size, uint8_t initByte, SharedRegionMerger *mergeObj);
    bool isInitialized() const { return initialized; }
    bool isReady() const { return ready; }

    SharedRegionImpl* addSharer(SharedRegionManager *manager);
    void removeSharer(SharedRegionImpl *sri);


    void modifyRegion(size_t offset, size_t length, const void *data);
    void publish();

    void updateState(bool finalize);

    const std::string& getKey() const { return myKey; }
    void* getMemory() { didBulk = true; return memory; }
    const void* getConstPtr() const { return memory; }
    size_t getSize() const { return apparentSize; }
    size_t getNumSharers() const { return shareCount; }

    bool shouldMerge() const { return (NULL != merger); }
    SharedRegionMerger* getMerger() { return merger; }
    /** Returns the size of the data to be transferred */
    RegionMergeInfo* getMergeInfo();

    void setProtected(bool readOnly);
};


class SharedRegionImpl : public SharedRegion {
    bool published;
    RegionInfo *region;
public:
    SharedRegionImpl(SharedRegionManager *manager, size_t id,
            size_t size, RegionInfo *region) : SharedRegion(manager, id, size),
        published(false), region(region)
    { }

    bool isPublished() const { return published; }
    void setPublished() { published = true; }
    RegionInfo* getRegion() const { return region; }
};



class SharedRegionManagerImpl : public SharedRegionManager {

    struct CommInfo_t {
        RegionInfo *region;
        std::vector<int> tgtRanks;
        std::vector<char> sendBuffer;
    };

    std::map<std::string, RegionInfo> regions;

protected:
    void modifyRegion(SharedRegion *sr, size_t offset, size_t length, const void *data);
    void* getMemory(SharedRegion *sr);
    const void* getConstPtr(const SharedRegion *sr) const;

public:
    SharedRegionManagerImpl();
    ~SharedRegionManagerImpl();

    virtual SharedRegion* getLocalSharedRegion(const std::string &key, size_t size, uint8_t initByte = 0);
    virtual SharedRegion* getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger = NULL, uint8_t initByte = 0);

    virtual void publishRegion(SharedRegion*);
    virtual bool isRegionReady(const SharedRegion*);
    virtual void shutdownSharedRegion(SharedRegion*);

    void updateState(bool finalize);
};


}
#endif
