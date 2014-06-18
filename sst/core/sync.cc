// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/sync.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"

#ifdef HAVE_MPI
#include <boost/mpi.hpp>
#endif

namespace SST {

    void
    SyncBase::setMaxPeriod(TimeConverter* period)
    {
        max_period = period;
        Simulation *sim = Simulation::getSimulation();
        SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
        setPriority(25);
        sim->insertActivity( next, this );        
    }
    
    template<class Archive>
    void
    SyncBase::serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
        ar & BOOST_SERIALIZATION_NVP(max_period);
        ar & BOOST_SERIALIZATION_NVP(exit);
    }

    void
    SyncBase::sendInitData_sync(Link* link, Event* init_data)
    {
        link->sendInitData_sync(init_data);
    }
    
    void
    SyncBase::finalizeConfiguration(Link* link)
    {
        link->finalizeConfiguration();
    }

    void
    SyncBase::print(const std::string& header, Output &out) const
    {
        out.output("%s Sync with period %" PRIu64 " to be delivered at %" PRIu64
                   " with priority %d\n",
                   header.c_str(), max_period->getFactor(), getDeliveryTime(), getPriority());
    }
    // Sync::Sync(TimeConverter* period) :
    //     Action(),
    //     exit(NULL)
    // {
    //     this->period = period;
    //     Simulation *sim = Simulation::getSimulation();
    //     SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
    //     setPriority(25);
    //     sim->insertActivity( next, this );
    // }

    ////   Sync
    
    Sync::Sync() :
        SyncBase()
    {
    }
    
    Sync::~Sync()
    {
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            delete i->second.first;
            delete i->second.second;
        }
        comm_map.clear();
        
        for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
            delete i->second;
        }
        link_map.clear();
    }
    
    ActivityQueue* Sync::registerLink(int rank, LinkId_t link_id, Link* link)
    {
        SyncQueue* queue;
        if ( comm_map.count(rank) == 0 ) {
            queue = comm_map[rank].first = new SyncQueue();
            comm_map[rank].second = new std::vector<Activity*>;
        } else {
            queue = comm_map[rank].first;
        }
	
        link_map[link_id] = link;
        return queue;
    }

    void
    Sync::finalizeLinkConfigurations() {
        for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
            // i->second->finalizeConfiguration();
            finalizeConfiguration(i->second);
        }
    }
    
    void
    Sync::execute(void)
    {

#ifdef HAVE_MPI
        std::vector<boost::mpi::request> pending_requests;

        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            pending_requests.push_back(comm.isend(i->first, 0, *(i->second.first->getVector())));
            pending_requests.push_back(comm.irecv(i->first, 0, *(i->second.second)));
        }
        boost::mpi::wait_all(pending_requests.begin(), pending_requests.end());

        Simulation* sim = Simulation::getSimulation();
        SimTime_t current_cycle = sim->getCurrentSimCycle();
        // Clear the SyncQueue used to send the data
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            i->second.first->clear();

            // Need to look through all the entries in the vector and
            // put them on the correct link
            std::vector<Activity*> *tmp = i->second.second;
            for (std::vector<Activity*>::iterator j = tmp->begin() ; j != tmp->end() ; ++j) {
                Event *ev = static_cast<Event*>(*j);
                link_map_t::iterator link = link_map.find(ev->getLinkId());
                if (link == link_map.end()) {
                    printf("Link not found in map!\n");
                    abort();
                } else {
                    // Need to figure out what the "delay" is for this event.
                    SimTime_t delay = ev->getDeliveryTime() - current_cycle;
                    link->second->send(delay,ev);
                }
            }
            // Clear the receive vector
            tmp->clear();
        }        

        // If we have an Exit object, fire it to see if we need end simulation
        if ( exit != NULL ) exit->check();
        
        SimTime_t next = sim->getCurrentSimCycle() + max_period->getFactor();
        // std::cout << next << std::endl;
        sim->insertActivity( next, this );
#endif
    }

    int
    Sync::exchangeLinkInitData(int msg_count)
    {
#ifdef HAVE_MPI
        std::vector<boost::mpi::request> pending_requests;
        
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            pending_requests.push_back(comm.isend(i->first, 0, i->second.first->getVector()));
            pending_requests.push_back(comm.irecv(i->first, 0, i->second.second));
        }
        boost::mpi::wait_all(pending_requests.begin(), pending_requests.end());

        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            // Clear the SyncQueue used to send the data
            i->second.first->clear();

            // Need to look through all the entries in the vector and
            // put them on the correct link
            std::vector<Activity*> *tmp = i->second.second;
            for (std::vector<Activity*>::iterator j = tmp->begin() ; j != tmp->end() ; ++j) {
                Event *ev = static_cast<Event*>(*j);
                link_map_t::iterator link = link_map.find(ev->getLinkId());
                if (link == link_map.end()) {
                    printf("Link not found in map!\n");
                    abort();
                } else {
                    // Need to figure out what the "delay" is for this event.
                    // link->second->sendInitData_sync(ev);
                    sendInitData_sync(link->second,ev);
                }
            }
            // Clear the receive vector
            tmp->clear();
        }

        // Do an allreduce to see if there were any messages sent
        boost::mpi::communicator world;
        int input = msg_count;
        int count;
        all_reduce( world, &input, 1, &count, std::plus<int>() );
        return count;
#else
        return 0;
#endif
    }
    
    template<class Archive>
    void
    Sync::serialize(Archive & ar, const unsigned int version)
    {
        printf("begin Sync::serialize\n");
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SyncBase);
        printf("  - Sync::period\n");
        // ar & BOOST_SERIALIZATION_NVP(period);
        printf("  - Sync::comm_map (%d)\n", (int) comm_map.size());
        ar & BOOST_SERIALIZATION_NVP(comm_map);
        printf("  - Sync::link_map (%d)\n", (int) link_map.size());
        ar & BOOST_SERIALIZATION_NVP(link_map);
        // don't serialize comm - let it be silently rebuilt at restart
        printf("end Sync::serialize\n");
    }
    
    ////   SyncB
    
    SyncB::SyncB() :
        SyncBase()
    {
    }
    
    SyncB::~SyncB()
    {
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            delete i->second.first;
            delete i->second.second;
        }
        comm_map.clear();
        
        for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
            delete i->second;
        }
        link_map.clear();
    }
    
    ActivityQueue* SyncB::registerLink(int rank, LinkId_t link_id, Link* link)
    {
        SyncQueue* queue;
        if ( comm_map.count(rank) == 0 ) {
            queue = comm_map[rank].first = new SyncQueue();
            comm_map[rank].second = new std::vector<Activity*>;
        } else {
            queue = comm_map[rank].first;
        }
	
        link_map[link_id] = link;
        return queue;
    }

    void
    SyncB::finalizeLinkConfigurations() {
        for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
            // i->second->finalizeConfiguration();
            finalizeConfiguration(i->second);
        }
    }
    
    void
    SyncB::execute(void)
    {
        // static Output tmp_debug("@r: @t:  ",5,-1,Output::FILE);
#ifdef HAVE_MPI
        std::vector<boost::mpi::request> pending_requests;

        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            pending_requests.push_back(comm.isend(i->first, 0, *(i->second.first->getVector())));
            pending_requests.push_back(comm.irecv(i->first, 0, *(i->second.second)));
        }
        boost::mpi::wait_all(pending_requests.begin(), pending_requests.end());

        Simulation* sim = Simulation::getSimulation();
        SimTime_t current_cycle = sim->getCurrentSimCycle();
        // Clear the SyncQueue used to send the data
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            i->second.first->clear();

            // Need to look through all the entries in the vector and
            // put them on the correct link
            std::vector<Activity*> *tmp = i->second.second;
            for (std::vector<Activity*>::iterator j = tmp->begin() ; j != tmp->end() ; ++j) {
                Event *ev = static_cast<Event*>(*j);
                link_map_t::iterator link = link_map.find(ev->getLinkId());
                if (link == link_map.end()) {
                    printf("Link not found in map!\n");
                    abort();
                } else {
                    // Need to figure out what the "delay" is for this event.
                    SimTime_t delay = ev->getDeliveryTime() - current_cycle;
                    link->second->send(delay,ev);
                }
            }
            // Clear the receive vector
            tmp->clear();
        }        

        // If we have an Exit object, fire it to see if we need end simulation
        if ( exit != NULL ) exit->check();

        // Check to see when the next event is scheduled, then do an
        // all_reduce with min operator and set next sync time to be
        // min + max_period.
        boost::mpi::communicator world;
        SimTime_t input = Simulation::getSimulation()->getNextActivityTime();;
        SimTime_t min_time;
        all_reduce( world, &input, 1, &min_time, boost::mpi::minimum<SimTime_t>() );

        // tmp_debug.output(CALL_INFO,"  my_time: %" PRIu64 ", min_time: %" PRIu64 "\n",input, min_time);
        
        SimTime_t next = min_time + max_period->getFactor();
        // std::cout << next << std::endl;
        sim->insertActivity( next, this );
#endif
    }

    int
    SyncB::exchangeLinkInitData(int msg_count)
    {
#ifdef HAVE_MPI
        std::vector<boost::mpi::request> pending_requests;
        
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            pending_requests.push_back(comm.isend(i->first, 0, i->second.first->getVector()));
            pending_requests.push_back(comm.irecv(i->first, 0, i->second.second));
        }
        boost::mpi::wait_all(pending_requests.begin(), pending_requests.end());

        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            // Clear the SyncQueue used to send the data
            i->second.first->clear();

            // Need to look through all the entries in the vector and
            // put them on the correct link
            std::vector<Activity*> *tmp = i->second.second;
            for (std::vector<Activity*>::iterator j = tmp->begin() ; j != tmp->end() ; ++j) {
                Event *ev = static_cast<Event*>(*j);
                link_map_t::iterator link = link_map.find(ev->getLinkId());
                if (link == link_map.end()) {
                    printf("Link not found in map!\n");
                    abort();
                } else {
                    // Need to figure out what the "delay" is for this event.
                    // link->second->sendInitData_sync(ev);
                    sendInitData_sync(link->second,ev);
                }
            }
            // Clear the receive vector
            tmp->clear();
        }

        // Do an allreduce to see if there were any messages sent
        boost::mpi::communicator world;
        int input = msg_count;
        int count;
        all_reduce( world, &input, 1, &count, std::plus<int>() );
        return count;
#else
        return 0;
#endif
    }
    
    template<class Archive>
    void
    SyncB::serialize(Archive & ar, const unsigned int version)
    {
        printf("begin Sync::serialize\n");
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SyncBase);
        printf("  - Sync::period\n");
        // ar & BOOST_SERIALIZATION_NVP(period);
        printf("  - Sync::comm_map (%d)\n", (int) comm_map.size());
        ar & BOOST_SERIALIZATION_NVP(comm_map);
        printf("  - Sync::link_map (%d)\n", (int) link_map.size());
        ar & BOOST_SERIALIZATION_NVP(link_map);
        // don't serialize comm - let it be silently rebuilt at restart
        printf("end Sync::serialize\n");
    }
    
} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::SyncBase::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::SyncBase)
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Sync::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Sync)
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::SyncB::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::SyncB)
