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

#include <sst/core/simulation.h>
#include <sst/core/sharedRegionImpl.h>
#include <sst/core/objectComms.h>


namespace SST {



bool SharedRegionMerger::merge(uint8_t *target, const uint8_t *newData, size_t size)
{
    return false;
}


bool SharedRegionMerger::merge(uint8_t *target, size_t size, const std::vector<ChangeSet> &changeSets)
{
    for ( auto &&i = changeSets.begin() ; i != changeSets.end() ; ++i ) {
        if ( i->offset + i->length > size ) return false;
        memcpy(&target[i->offset], i->data, i->length);
    }
    return true;
}



bool SharedRegionInitializedMerger::merge(uint8_t *target, const uint8_t *newData, size_t size)
{
    for ( size_t i = 0; i < size ; i++ ) {
        target[i] = (newData[i] != defVal) ? newData[i] : target[i];
    }
    return true;
}


RegionInfo::~RegionInfo(void)
{
    if ( memory ) {
        setProtected(false);
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


bool RegionInfo::initialize(const std::string &key, size_t size, uint8_t initByte, SharedRegionMerger *mergeObj)
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
    memset(memory, initByte, realSize);

    myKey = key;
    shareCount = 0;
    publishCount = 0;
    apparentSize = size;
    merger = mergeObj;
    initialized = true;
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

void RegionInfo::modifyRegion(size_t offset, size_t length, const void *data)
{
    uint8_t *ptr = static_cast<uint8_t*>(memory);
    memcpy(ptr + offset, data, length);

    changesets.emplace_back(offset, length, ptr + offset);
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
        setProtected(true);
        ready = true;
    }
}


RegionInfo::RegionMergeInfo* RegionInfo::getMergeInfo()
{
    int rank = Simulation::getSimulation()->getRank();
    if ( didBulk ) {
        return new BulkMergeInfo(rank, myKey, memory, apparentSize);
    } else if ( !changesets.empty() ) {
        return new ChangeSetMergeInfo(rank, myKey, changesets);
    }
    return new RegionMergeInfo(rank, myKey);
}


void RegionInfo::setProtected(bool readOnly)
{
    if ( readOnly )
        mprotect(memory, realSize, PROT_READ);
    else
        mprotect(memory, realSize, PROT_READ|PROT_WRITE);
}


SharedRegionManagerImpl::SharedRegionManagerImpl()
{
}


SharedRegionManagerImpl::~SharedRegionManagerImpl()
{
}




SharedRegion* SharedRegionManagerImpl::getLocalSharedRegion(const std::string &key, size_t size, uint8_t initByte)
{
    RegionInfo& ri = regions[key];
    if ( ri.isInitialized() ) {
        if ( ri.getSize() != size ) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Mismatched Sizes!\n");
    } else {
        if ( false == ri.initialize(key, size, initByte, NULL) ) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Shared Region Initialized Failed!\n");
    }
    return ri.addSharer(this);
}


SharedRegion* SharedRegionManagerImpl::getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger, uint8_t initByte)
{
    RegionInfo& ri = regions[key];
    if ( ri.isInitialized() ) {
        if ( ri.getSize() != size ) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Mismatched Sizes!\n");
    } else {
        if ( false == ri.initialize(key, size, initByte, merger) ) Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Shared Region Initialized Failed!\n");
    }
    return ri.addSharer(this);
}


void SharedRegionManagerImpl::modifyRegion(SharedRegion *sr, size_t offset, size_t length, const void *data)
{
    SharedRegionImpl *sri = static_cast<SharedRegionImpl*>(sr);
    RegionInfo *ri = sri->getRegion();
    ri->modifyRegion(offset, length, data);
}


void* SharedRegionManagerImpl::getMemory(SharedRegion *sr)
{
    SharedRegionImpl *sri = static_cast<SharedRegionImpl*>(sr);
    RegionInfo *ri = sri->getRegion();
    return ri->getMemory();
}


const void* SharedRegionManagerImpl::getConstPtr(const SharedRegion *sr) const
{
    const SharedRegionImpl *sri = static_cast<const SharedRegionImpl*>(sr);
    RegionInfo *ri = sri->getRegion();
    return ri->getConstPtr();
}



void SharedRegionManagerImpl::updateState(bool finalize)
{

    if ( finalize ) {
#ifdef SST_CONFIG_HAVE_MPI
        int myRank = Simulation::getSimulation()->getRank();
        if ( Simulation::getSimulation()->getNumRanks() > 1 ) {

            std::map<std::string, CommInfo_t> commInfo;

            std::set<std::pair<std::string, size_t> > myKeys;
            std::vector<std::set<std::pair<std::string, size_t> > > allKeysVec;
            for ( auto &&rii = regions.begin() ; rii != regions.end() ; ++rii ) {
                if ( rii->second.shouldMerge() ) {
                    CommInfo_t ci;
                    ci.region = &(rii->second);

                    RegionInfo::RegionMergeInfo *rmi = rii->second.getMergeInfo();
                    ci.sendBuffer = Comms::serialize(rmi);

                    myKeys.insert(std::make_pair(rii->first, ci.sendBuffer.size()));
                    commInfo[rii->first] = ci;
                }
            }

            Comms::all_gather(myKeys, allKeysVec);


            size_t numRecv = 0;
            for ( size_t rank = 0 ; rank < allKeysVec.size() ; rank++ ) {
                if ( rank == (size_t)myRank ) continue;
                // Foreach key in not-me-rank, see if I have it, too.
                // If so, record as such for future merging
                for ( auto && j = allKeysVec[rank].begin() ; j != allKeysVec[rank].end() ; ++j ) {
                    auto mItr = regions.find(j->first);
                    if ( mItr != regions.end() ) {
                        numRecv++;
                    }
                }
            }

            if ( numRecv > 0 ) {

                std::vector<std::vector<char> > receives(numRecv);
                MPI_Request recvReqs[numRecv];
                MPI_Request sendReqs[numRecv];
                size_t req = 0;
                for ( size_t rank = 0 ; rank < allKeysVec.size() ; rank++ ) {
                    if ( rank == (size_t)myRank ) continue;
                    // Foreach key in rank, see if I have it, too.
                    // If so, record as such for future merging
                    for ( auto && j = allKeysVec[rank].begin() ; j != allKeysVec[rank].end() ; ++j ) {
                        auto mItr = regions.find(j->first);
                        if ( mItr != regions.end() ) {
                            CommInfo_t &ci = commInfo[mItr->first];
                            ci.tgtRanks.push_back(rank);

                            // Post receive
                            receives[req].resize(j->second);
                            MPI_Irecv(receives[req].data(), receives[req].size(), MPI_BYTE, rank, 0, MPI_COMM_WORLD, &recvReqs[req]);
                            ++req;
                        }
                    }
                }

                // Sends merge information
                req = 0;
                for ( auto && ciItr = commInfo.begin() ; ciItr != commInfo.end() ; ++ciItr ) {
                    CommInfo_t &ci = ciItr->second;
                    for ( auto && rank = ci.tgtRanks.begin() ; rank != ci.tgtRanks.end() ; ++rank ) {
                        MPI_Isend(ci.sendBuffer.data(), ci.sendBuffer.size(), MPI_BYTE, *rank, 0, MPI_COMM_WORLD, &sendReqs[req]);
                        ++req;
                    }
                }

                // Wait for Recvs, and merge
                for ( size_t i = 0 ; i < numRecv ; i++ ) {
                    int index;
                    MPI_Status status;
                    MPI_Waitany(numRecv, recvReqs, &index, &status);

                    RegionInfo::RegionMergeInfo *rmi = Comms::deserialize<RegionInfo::RegionMergeInfo>(receives[index]);

                    RegionInfo &ri = regions[rmi->getKey()];
                    ri.setProtected(false);
                    rmi->merge(&ri);

                    delete rmi;
                }

                // WaitAll on sends
                MPI_Waitall(numRecv, sendReqs, MPI_STATUSES_IGNORE);

            }

        }
#endif
    }

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


