// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/shared/sharedObject.h"

#include "sst/core/objectComms.h"
#include "sst/core/output.h"
#include "sst/core/shared/sharedArray.h"
#include "sst/core/shared/sharedMap.h"
#include "sst/core/shared/sharedSet.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/warnmacros.h"

namespace SST {
namespace Shared {

namespace Private {
Output&
getSimulationOutput()
{
    return Simulation_impl::getSimulationOutput();
}

Simulation*
getSimulation()
{
    return Simulation_impl::getSimulation();
}

} // namespace Private

SharedObjectDataManager SharedObject::manager;
std::mutex              SharedObjectDataManager::mtx;

std::mutex SharedObjectDataManager::update_mtx;

void
SharedObjectDataManager::updateState(bool finalize)
{
    std::lock_guard<std::mutex> lock(update_mtx);

#ifdef SST_CONFIG_HAVE_MPI
    // Exchange data between ranks
    if ( Simulation_impl::getSimulation()->getNumRanks().rank > 1 ) {
        int                                 myRank = Simulation_impl::getSimulation()->getRank().rank;
        // Create a vector off all my changesets
        std::vector<SharedObjectChangeSet*> myChanges;

        // Create a vector to check to see if regions are fully
        // published
        std::vector<std::pair<std::string, bool>> myFullPub;
        for ( auto x : shared_data ) {
            myChanges.push_back(x.second->getChangeSet());
            myFullPub.emplace_back(x.second->getName(), x.second->getPublishCount() == x.second->getShareCount());
        }

        std::vector<std::vector<SharedObjectChangeSet*>>       allChanges;
        std::vector<std::vector<std::pair<std::string, bool>>> allFullyPub;

        // Get and apply all the changes
        Comms::all_gather(myChanges, allChanges);
        for ( size_t i = 0; i < allChanges.size(); ++i ) {
            if ( i == (size_t)myRank ) continue;

            for ( size_t j = 0; j < allChanges[i].size(); ++j ) {
                auto cs = allChanges[i][j];
                cs->applyChanges(this);
                delete cs;
            }
        }
        for ( auto x : shared_data ) {
            x.second->getChangeSet()->clear();
        }

        // See if the SharedObjects are ready
        Comms::all_gather(myFullPub, allFullyPub);
        std::map<std::string, bool> pub_map;
        for ( auto x : allFullyPub ) {
            for ( auto y : x ) {
                auto it = pub_map.find(y.first);
                if ( it == pub_map.end() ) { pub_map[y.first] = y.second; }
                else {
                    it->second = it->second & y.second;
                }
            }
        }

        for ( auto x : pub_map ) {
            shared_data[x.first]->fully_published = x.second;
        }
    }
    else {
#endif
        // We don't need to exchange data, but we do need to do the
        // ready flags
        for ( auto x : shared_data ) {
            if ( x.second->getPublishCount() == x.second->getShareCount() )
                x.second->fully_published = true;
            else
                x.second->fully_published = false;
        }
#ifdef SST_CONFIG_HAVE_MPI
    }
#endif
    if ( finalize ) {
        // Need to lock the objects and mark them as fully published
        for ( auto x : shared_data ) {
            x.second->lock();
            x.second->fully_published = true;
        }
        locked = true;
    }
}

} // namespace Shared
} // namespace SST
