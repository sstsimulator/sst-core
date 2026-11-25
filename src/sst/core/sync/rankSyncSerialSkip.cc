// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/sync/rankSyncSerialSkip.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/profile.h"
#include "sst/core/profile/syncProfileTool.h"
#include "sst/core/serialization/serializer.h"
#include "sst/core/simulation.h"
#include "sst/core/sst_mpi.h"
#include "sst/core/sync/syncQueue.h"
#include "sst/core/timeConverter.h"

#include <atomic>

#if SST_EVENT_PROFILING
#define SST_EVENT_PROFILE_START auto event_profile_start = std::chrono::high_resolution_clock::now();

#define SST_EVENT_PROFILE_STOP                                                                                  \
    auto event_profile_stop = std::chrono::high_resolution_clock::now();                                        \
    auto event_profile_count =                                                                                  \
        std::chrono::duration_cast<std::chrono::nanoseconds>(event_profile_stop - event_profile_start).count(); \
    sim->incrementSerialCounters(event_profile_count);
#else
#define SST_EVENT_PROFILE_START
#define SST_EVENT_PROFILE_STOP
#endif


namespace SST {

// Static Data Members
SimTime_t RankSyncSerialSkip::myNextSyncTime = 0;

RankSyncSerialSkip::RankSyncSerialSkip(RankInfo num_ranks) :
    RankSync(num_ranks),
    mpiWaitTime(0.0),
    deserializeTime(0.0)
{
    max_period     = Simulation::getSimulation()->getMinPartTC().getFactor();
    myNextSyncTime = max_period;
}

RankSyncSerialSkip::~RankSyncSerialSkip()
{
    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        delete i->second.squeue;
    }
    comm_map.clear();

    if ( mpiWaitTime > 0.0 || deserializeTime > 0.0 )
        Output::getDefaultObject().verbose(CALL_INFO, 1, 0,
            "RankSyncSerialSkip mpiWait: %lg sec  deserializeWait:  %lg sec\n", mpiWaitTime, deserializeTime);
}

ActivityQueue*
RankSyncSerialSkip::registerLink(const RankInfo& to_rank, const RankInfo& UNUSED(from_rank), Link* link)
{
    std::scoped_lock slock(lock);

    RankSyncQueue* queue;
    if ( comm_map.count(to_rank.rank) == 0 ) {
        queue = comm_map[to_rank.rank].squeue = new RankSyncQueue(to_rank);
        comm_map[to_rank.rank].remote_size    = RankSyncRecvBuffer::initialSize();
        comm_map[to_rank.rank].squeue->setProfileTools(profile_tools_);
    }
    else {
        queue = comm_map[to_rank.rank].squeue;
    }

    link_maps[to_rank.rank].emplace_back(link->getId(), reinterpret_cast<uintptr_t>(link));
#ifdef __SST_DEBUG_EVENT_TRACKING__
    link->setSendingComponentInfo("SYNC", "SYNC", "");
#endif
    return queue;
}

void
RankSyncSerialSkip::setRestartTime(SimTime_t time)
{
    if ( Simulation::getSimulation()->getRank().thread == 0 ) {
        myNextSyncTime = time;
    }
}

void
RankSyncSerialSkip::finalizeLinkConfigurations()
{}

void
RankSyncSerialSkip::prepareForComplete()
{}

void
RankSyncSerialSkip::setSignals(int end, int usr, int alrm)
{
    sig_end_  = end;
    sig_usr_  = usr;
    sig_alrm_ = alrm;
}

bool
RankSyncSerialSkip::getSignals(int& end, int& usr, int& alrm)
{
    end  = sig_end_;
    usr  = sig_usr_;
    alrm = sig_alrm_;
    return sig_end_ || sig_usr_ || sig_alrm_;
}

void
RankSyncSerialSkip::setShutdownFlags(bool enter_shutdown, Simulation::ShutdownMode_t shutdown_mode)
{
    // This can be set from any thread
    if ( enter_shutdown ) {
        enter_shutdown_.store(enter_shutdown);
        shutdown_mode_.store(static_cast<unsigned>(shutdown_mode));
    }
}

void
RankSyncSerialSkip::setCkptFlag(bool generate_ckpt)
{
    if ( generate_ckpt ) generate_ckpt_.store(true);
}

void
RankSyncSerialSkip::setFlags(bool enter_interactive, bool enter_shutdown, Simulation::ShutdownMode_t shutdown_mode)
{
    // This can be set from any thread
    if ( enter_interactive ) enter_interactive_.store(enter_interactive);

    setShutdownFlags(enter_shutdown, shutdown_mode);
}

void
RankSyncSerialSkip::getShutdownFlags(bool& enter_shutdown, Simulation::ShutdownMode_t& shutdown_mode)
{
    enter_shutdown = enter_shutdown_.load();
    switch ( shutdown_mode_ ) {
    case 0:
        shutdown_mode = Simulation::ShutdownMode_t::SHUTDOWN_CLEAN;
        break;
    case 1:
        shutdown_mode = Simulation::ShutdownMode_t::SHUTDOWN_SIGNAL;
        break;
    case 2:
        shutdown_mode = Simulation::ShutdownMode_t::SHUTDOWN_EMERGENCY;
        break;
    }
}

void
RankSyncSerialSkip::getCkptFlag(bool& generate_ckpt)
{
    generate_ckpt = generate_ckpt_.load();
}

void
RankSyncSerialSkip::getFlags(bool& enter_interactive, bool& enter_shutdown, Simulation::ShutdownMode_t& shutdown_mode)
{
    enter_interactive = enter_interactive_.load();
    getShutdownFlags(enter_shutdown, shutdown_mode);
}

void
RankSyncSerialSkip::clearFlags()
{
    enter_interactive_.store(false);
    enter_shutdown_.store(false);
    shutdown_mode_.store(0);
    generate_ckpt_.store(false);
}

uint64_t
RankSyncSerialSkip::getDataSize() const
{
    size_t count = 0;
    for ( comm_map_t::const_iterator it = comm_map.begin(); it != comm_map.end(); ++it ) {
        count += (it->second.squeue->getDataSize() + it->second.rbuf.getDataSize());
    }
    return count;
}

void
RankSyncSerialSkip::execute(int thread)
{
    if ( thread == 0 ) {
        exchange();
    }
}

void
RankSyncSerialSkip::exchange()
{
#ifdef SST_CONFIG_HAVE_MPI
    exchangeEvents();

    // If we have an Exit object, fire it to see if we need end simulation
    // if ( exit != nullptr ) exit->check();

    // Check to see when the next event is scheduled, then do an
    // all_reduce with min operator and set next sync time to be
    // min + max_period.

    // Need to get the local minimum, then do a global minimum
    // SimTime_t input = Simulation::getSimulation()->getNextActivityTime();
    SimTime_t input = Simulation::getLocalMinimumNextActivityTime();
    SimTime_t min_time;
    MPI_Allreduce(&input, &min_time, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD);

    myNextSyncTime = min_time + max_period;

    int32_t local_signals[3]  = { sig_end_, sig_usr_, sig_alrm_ };
    int32_t global_signals[3] = { 0, 0, 0 };
    MPI_Allreduce(&local_signals, &global_signals, 3, MPI_INT32_T, MPI_MAX, MPI_COMM_WORLD);

    sig_end_  = global_signals[0];
    sig_usr_  = global_signals[1];
    sig_alrm_ = global_signals[2];

    int32_t local_flags[4]  = { static_cast<int32_t>(enter_interactive_), static_cast<int32_t>(enter_shutdown_),
         static_cast<int32_t>(shutdown_mode_), static_cast<int32_t>(generate_ckpt_) };
    int32_t global_flags[4] = { 0, 0, 0, 0 };
    MPI_Allreduce(&local_flags, &global_flags, 4, MPI_INT32_T, MPI_MAX, MPI_COMM_WORLD);

    enter_interactive_ = global_flags[0];
    enter_shutdown_    = global_flags[1];
    shutdown_mode_     = global_flags[2];
    generate_ckpt_     = global_flags[3];

#endif
}

void
RankSyncSerialSkip::exchangeEvents()
{
#ifdef SST_CONFIG_HAVE_MPI
    // Each send uses one request per chunk, plus one if the remote buffer has to grow
    std::vector<MPI_Request> sreqs;
    std::vector<MPI_Request> rreqs(comm_map.size());
    int                      rreq_count = 0;

    Simulation* sim = Simulation::getSimulation();

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {

        SST_EVENT_PROFILE_START

        // Do all the sends
        // Serialize the data in the syncQueue
        i->second.squeue->getData();

        SST_EVENT_PROFILE_STOP

        i->second.squeue->sendData(i->first /*dest*/, 1, 2, i->second.remote_size, sreqs);

        // Post all the receives for the first messages
        i->second.rbuf.postFirstRecv(i->first, 1, &rreqs[rreq_count++]);
    }

    // Wait for all the first messages, then post the receives for the remaining messages and wait for those
    SimTime_t current_cycle = sim->getCurrentSimCycle();

    auto waitStart = SST::Core::Profile::now();
    MPI_Waitall(rreq_count, rreqs.data(), MPI_STATUSES_IGNORE);

    rreqs.clear();
    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        i->second.rbuf.postRemainingRecvs(i->first, 2, rreqs);
    }
    MPI_Waitall(rreqs.size(), rreqs.data(), MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        // Deserialize all the events
        auto deserialStart = SST::Core::Profile::now();

        std::vector<Activity*> activities;
        i->second.rbuf.unpack(activities);

        deserializeTime += SST::Core::Profile::getElapsed(deserialStart);

        for ( unsigned int j = 0; j < activities.size(); j++ ) {
            Event*    ev    = static_cast<Event*>(activities[j]);
            SimTime_t delay = ev->getDeliveryTime() - current_cycle;
            getDeliveryLink(ev)->send(delay, ev);
        }

        activities.clear();
    }

    // Clear the RankSyncQueues used to send the data after all the sends have completed
    waitStart = SST::Core::Profile::now();
    MPI_Waitall(sreqs.size(), sreqs.data(), MPI_STATUSES_IGNORE);
    mpiWaitTime += SST::Core::Profile::getElapsed(waitStart);

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        i->second.squeue->clear();
    }
#endif
}

void
RankSyncSerialSkip::exchangeSetupData(int UNUSED_WO_MPI(thread))
{
#ifdef SST_CONFIG_HAVE_MPI
    if ( thread != 0 ) return;

    // Deliver the events sent during setup(), then free the buffers, which may have grown large during init() and
    // setup(). Every rank does this at the same time, so the senders' and receivers' buffer sizes stay consistent.
    exchangeEvents();
    releaseBuffers();
#endif
}

void
RankSyncSerialSkip::releaseBuffers()
{
    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        i->second.squeue->releaseBuffer();
        i->second.rbuf.reset();
        i->second.remote_size = RankSyncRecvBuffer::initialSize();
    }
}

void
RankSyncSerialSkip::exchangeLinkUntimedData(int UNUSED_WO_MPI(thread), std::atomic<int>& UNUSED_WO_MPI(msg_count))
{
#ifdef SST_CONFIG_HAVE_MPI
    if ( thread != 0 ) {
        return;
    }

    // Each send uses one request per chunk, plus one if the remote buffer has to grow
    std::vector<MPI_Request> sreqs;
    std::vector<MPI_Request> rreqs(comm_map.size());
    int                      rreq_count = 0;

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {

        // Do all the sends
        // Serialize the data in the syncQueue
        i->second.squeue->getData();
        i->second.squeue->sendData(i->first /*dest*/, 1, 2, i->second.remote_size, sreqs);

        // Post all the receives for the first messages
        i->second.rbuf.postFirstRecv(i->first, 1, &rreqs[rreq_count++]);
    }

    // Wait for all the first messages, then post the receives for the remaining messages and wait for those
    MPI_Waitall(rreq_count, rreqs.data(), MPI_STATUSES_IGNORE);

    rreqs.clear();
    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        i->second.rbuf.postRemainingRecvs(i->first, 2, rreqs);
    }
    MPI_Waitall(rreqs.size(), rreqs.data(), MPI_STATUSES_IGNORE);

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {

        // Deserialize all the events
        std::vector<Activity*> activities;
        i->second.rbuf.unpack(activities);
        for ( unsigned int j = 0; j < activities.size(); j++ ) {

            Event* ev = static_cast<Event*>(activities[j]);
            sendUntimedData_sync(getDeliveryLink(ev), ev);
        }
    }

    // Clear the RankSyncQueues used to send the data after all the sends have completed
    MPI_Waitall(sreqs.size(), sreqs.data(), MPI_STATUSES_IGNORE);

    for ( comm_map_t::iterator i = comm_map.begin(); i != comm_map.end(); ++i ) {
        i->second.squeue->clear();
    }

    // Do an allreduce to see if there were any messages sent
    int input = msg_count;

    int count;
    MPI_Allreduce(&input, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    msg_count = count;
#endif
}

void
RankSyncSerialSkip::setProfileToolList(Profile::SyncProfileToolList* profile_tools)
{
    profile_tools_ = profile_tools;
}


int                   RankSyncSerialSkip::sig_end_(0);
int                   RankSyncSerialSkip::sig_usr_(0);
int                   RankSyncSerialSkip::sig_alrm_(0);
std::atomic<bool>     RankSyncSerialSkip::enter_interactive_(false);
std::atomic<bool>     RankSyncSerialSkip::enter_shutdown_(false);
std::atomic<unsigned> RankSyncSerialSkip::shutdown_mode_(0);
std::atomic<bool>     RankSyncSerialSkip::generate_ckpt_(false);

} // namespace SST
