// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_SHAREDREGIONIMPL_H
#define SST_CORE_CORE_SHAREDREGIONIMPL_H

#include "sst/core/output.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sharedRegion.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace SST {

class SharedRegionImpl;

class ChangeSet : public SST::Core::Serialization::serializable
{
public:
    ChangeSet() {}
    size_t             offset;
    size_t             length;
    /*const*/ uint8_t* data;

    ChangeSet(size_t offset, size_t length, /*const*/ uint8_t* data = nullptr) :
        offset(offset),
        length(length),
        data(data)
    {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        ser& offset;
        // ser & length;
        // if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        //     data = new uint8_t[length];
        // }
        // for ( int i = 0; i < length; i++ ) {
        //     ser & data[i];
        // }
        ser& SST::Core::Serialization::array(data, length);
        // ser.binary(data,length);
    }

    ImplementSerializable(SST::ChangeSet)
};

class RegionInfo
{
public:
    class RegionMergeInfo : public SST::Core::Serialization::serializable
    {

    protected:
        int         rank;
        std::string key;
        size_t      length;
        uint8_t     initByte;

        virtual void check_size(RegionInfo* ri)
        {
            // Check to see if sizes match.

            // If the merge info has zero length, then it doesn't know
            // the size, so just return
            if ( length == 0 ) return;

            ri->setSize(length, initByte);
        }

    public:
        RegionMergeInfo() {}
        RegionMergeInfo(int rank, const std::string& key, size_t length, uint8_t initByte) :
            rank(rank),
            key(key),
            length(length),
            initByte(initByte)
        {}
        virtual ~RegionMergeInfo() {}

        virtual bool merge(RegionInfo* ri)
        {
            check_size(ri);
            return true;
        }
        const std::string& getKey() const { return key; }
        size_t             getLength() const { return length; }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            ser& rank;
            ser& key;
            ser& length;
            ser& initByte;
        }

        ImplementSerializable(SST::RegionInfo::RegionMergeInfo)
    };

    class BulkMergeInfo : public RegionMergeInfo
    {
    protected:
        void* data;

    public:
        BulkMergeInfo() : RegionMergeInfo() {}
        BulkMergeInfo(int rank, const std::string& key, void* data, size_t length, uint8_t initByte) :
            RegionMergeInfo(rank, key, length, initByte),
            data(data)
        {}

        bool merge(RegionInfo* ri) override
        {
            check_size(ri);
            bool ret = ri->getMerger()->merge((uint8_t*)ri->getMemory(), (const uint8_t*)data, length);
            free(data);
            return ret;
        }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            RegionInfo::RegionMergeInfo::serialize_order(ser);

            // if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            //     data = malloc(length);
            // }
            // for ( int i = 0; i < length; i++ ) {
            //     ser & ((uint8_t*)data)[i];
            // }
            ser& SST::Core::Serialization::array(data, length);
            // ser.binary(data,length);
        }

        ImplementSerializable(SST::RegionInfo::BulkMergeInfo)
    };

    class ChangeSetMergeInfo : public RegionMergeInfo
    {
    protected:
        // std::vector<SharedRegionMerger::ChangeSet> changeSets;
        std::vector<ChangeSet> changeSets;

    public:
        ChangeSetMergeInfo() : RegionMergeInfo() {}
        ChangeSetMergeInfo(
            int rank, const std::string& key, size_t length, uint8_t initByte, std::vector<ChangeSet>& changeSets) :
            RegionMergeInfo(rank, key, length, initByte),
            changeSets(changeSets)
        {}
        bool merge(RegionInfo* ri) override
        {
            check_size(ri);
            return ri->getMerger()->merge((uint8_t*)ri->getMemory(), ri->getSize(), changeSets);
        }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            RegionInfo::RegionMergeInfo::serialize_order(ser);
            ser& changeSets;
        }

        ImplementSerializable(SST::RegionInfo::ChangeSetMergeInfo)
    };

private:
    std::string myKey;
    size_t      realSize;
    size_t      apparentSize;
    void*       memory;

    size_t shareCount;
    size_t publishCount;

    std::vector<SharedRegionImpl*> sharers;

    SharedRegionMerger*    merger; // If null, no multi-rank merging
    // std::vector<SharedRegionMerger::ChangeSet> changesets;
    std::vector<ChangeSet> changesets;

    bool    didBulk;
    bool    initialized;
    bool    ready;
    uint8_t initByte;

public:
    RegionInfo() :
        realSize(0),
        apparentSize(0),
        memory(nullptr),
        shareCount(0),
        publishCount(0),
        merger(nullptr),
        didBulk(false),
        initialized(false),
        ready(false),
        initByte(0)
    {}
    ~RegionInfo();
    bool initialize(const std::string& key, size_t size, uint8_t initByte_in, SharedRegionMerger* mergeObj);
    bool setSize(size_t size, uint8_t initByte_in);
    bool isInitialized() const { return initialized; }
    bool isReady() const { return ready; }

    SharedRegionImpl* addSharer(SharedRegionManager* manager);
    void              removeSharer(SharedRegionImpl* sri);

    void modifyRegion(size_t offset, size_t length, const void* data);
    void publish();

    void updateState(bool finalize);

    const std::string& getKey() const { return myKey; }
    void*              getMemory()
    {
        didBulk = true;
        return memory;
    }
    const void* getConstPtr() const { return memory; }
    size_t      getSize() const { return apparentSize; }
    size_t      getNumSharers() const { return shareCount; }

    bool                shouldMerge() const { return (nullptr != merger); }
    SharedRegionMerger* getMerger() { return merger; }
    /** Returns the size of the data to be transferred */
    RegionMergeInfo*    getMergeInfo();

    void setProtected(bool readOnly);
};

class SharedRegionImpl : public SharedRegion
{
    bool        published;
    RegionInfo* region;

protected:
    size_t getSize() { return region->getSize(); }

public:
    SharedRegionImpl(SharedRegionManager* manager, size_t id, RegionInfo* region) :
        SharedRegion(manager, id),
        published(false),
        region(region)
    {}

    bool        isPublished() const { return published; }
    void        setPublished() { published = true; }
    RegionInfo* getRegion() const { return region; }
    void        notifySetSize()
    {
        if ( deferred_pointer != nullptr ) {
            deferred_pointer->setPointer(getPtr<const void*>());
            delete deferred_pointer;
            deferred_pointer = nullptr;
        }
    }
};

class SharedRegionManagerImpl : public SharedRegionManager
{

    struct CommInfo_t
    {
        RegionInfo*       region;
        std::vector<int>  tgtRanks;
        std::vector<char> sendBuffer;
    };

    std::map<std::string, RegionInfo> regions;
    std::mutex                        mtx;

protected:
    void        modifyRegion(SharedRegion* sr, size_t offset, size_t length, const void* data) override;
    void*       getMemory(SharedRegion* sr) override;
    const void* getConstPtr(const SharedRegion* sr) const override;
    size_t      getSize(const SharedRegion* sr) const override;

public:
    SharedRegionManagerImpl();
    ~SharedRegionManagerImpl();

    virtual SharedRegion* getLocalSharedRegion(const std::string& key, size_t size, uint8_t initByte = 0) override;
    virtual SharedRegion* getGlobalSharedRegion(
        const std::string& key, size_t size, SharedRegionMerger* merger = nullptr, uint8_t initByte = 0) override;

    virtual void publishRegion(SharedRegion*) override;
    virtual bool isRegionReady(const SharedRegion*) override;
    virtual void shutdownSharedRegion(SharedRegion*) override;

    void updateState(bool finalize) override;
};

} // namespace SST

#endif // SST_CORE_CORE_SHAREDREGIONIMPL_H
