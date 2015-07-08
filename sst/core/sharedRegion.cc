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


#include <sst_config.h>
#include <sst/core/serialization.h>

#include <string>
#include <vector>
#include <set>
#include <map>

#include <sys/types.h>

#include <sst/core/sharedRegionImpl.h>
#include <sst/core/simulation.h>

namespace SST {


RegionInfo::~RegionInfo(void)
{
    if ( memory ) {
        mprotect(memory, realSize, PROT_READ|PROT_WRITE);
        free(memory);
        memory = NULL;
    }
    initialized = ready = false;
    size_t nSharers = sharers.size();
    for ( size_t i = 0 ; i < nSharers ; i++ ) {
        if ( sharers[i] ) {
            delete sharers[i];
            sharers[i] = NULL;
            --shareCount;
        }
    }
}


bool RegionInfo::initialize(const std::string &key, size_t size, SharedRegionMerger *mergeObj)
{
    if ( initialized ) return true;

    size_t pagesize = (size_t)sysconf(_SC_PAGE_SIZE);
    size_t npages = size / pagesize;
    if ( size % pagesize ) npages++;
    realSize = npages * pagesize;

    int ret = posix_memalign(&memory, pagesize, realSize);
    if ( ret ) {
        errno = ret;
        return false;
    }

    myKey = key;
    shareCount = 0;
    publishCount = 0;
    apparentSize = size;
    merger = mergeObj;
    initialized = true;
    ranks.insert(Simulation::getSimulation()->getRank());
    return true;

}


SharedRegionImpl* RegionInfo::addSharer(SharedRegionManager *manager)
{
    size_t id = sharers.size();
    SharedRegionImpl *sr = new SharedRegionImpl(manager, id, apparentSize, this);
    sharers.push_back(sr);
    shareCount++;
    return sr;
}

void RegionInfo::removeSharer(SharedRegionImpl *sri)
{
    size_t nShare = sharers.size();
    for ( size_t i = 0 ; i < nShare ; i++ ) {
        if ( sharers[i] == sri ) {
            delete sri;
            sharers[i] = NULL;
            shareCount--;
        }
    }
}



void RegionInfo::publish(void)
{
    publishCount++;
}


void RegionInfo::updateState(bool finalize)
{
    if ( !initialized ) return;
    if ( ready ) return;

    /* TODO:  Sync across ranks if necessary */

    if ( finalize ) {
        Output &out = Simulation::getSimulation()->getSimulationOutput();
        out.output(CALL_INFO, "WARNING:  SharedRegion [%s] was not fully published!  Forcing finalization.\n", myKey.c_str());
        // Force below check to pass
        publishCount = shareCount;
    }

    if ( shareCount == publishCount ) {
        mprotect(memory, realSize, PROT_READ);
        ready = true;
    }
}







SharedRegionManagerImpl::SharedRegionManagerImpl()
{
}


SharedRegionManagerImpl::~SharedRegionManagerImpl()
{
}




SharedRegion* SharedRegionManagerImpl::getLocalSharedRegion(const std::string &key, size_t size)
{
    RegionInfo& ri = regions[key];
    if ( ri.isInitialized() ) {
        if ( ri.getSize() != size ) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Mismatched Sizes!\n");
    } else {
        if ( false == ri.initialize(key, size, NULL) ) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Shared Region Initialized Failed!\n");
    }
    return ri.addSharer(this);
}


SharedRegion* SharedRegionManagerImpl::getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger)
{
    RegionInfo& ri = regions[key];
    if ( ri.isInitialized() ) {
        if ( ri.getSize() != size ) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Mismatched Sizes!\n");
    } else {
        if ( false == ri.initialize(key, size, merger) ) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Shared Region Initialized Failed!\n");
    }
    return ri.addSharer(this);
}



void SharedRegionManagerImpl::updateState(bool finalize)
{
    for ( auto &&rii = regions.begin() ; rii != regions.end() ; ++rii ) {
        RegionInfo &ri = rii->second;
        ri.updateState(finalize);
    }
}



void SharedRegionManagerImpl::publishRegion(SharedRegion *sr)
{
    SharedRegionImpl *sri = static_cast<SharedRegionImpl*>(sr);
    if ( !sri->isPublished() ) {
        RegionInfo *ri = sri->getRegion();
        sri->setPublished();
        ri->publish();
    }
}


bool SharedRegionManagerImpl::isRegionReady(const SharedRegion *sr)
{
    const SharedRegionImpl *sri = static_cast<const SharedRegionImpl*>(sr);
    return sri->getRegion()->isReady();
}


void SharedRegionManagerImpl::shutdownSharedRegion(SharedRegion *sr)
{
    SharedRegionImpl *sri = static_cast<SharedRegionImpl*>(sr);
    RegionInfo *ri = sri->getRegion();
    ri->removeSharer(sri);  // sri is now deleted

    if ( 0 == ri->getNumSharers() ) {
        auto itr = regions.find(ri->getKey());
        regions.erase(itr);
    }
}


}
