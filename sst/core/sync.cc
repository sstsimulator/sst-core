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
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"

#ifdef HAVE_MPI
#include <boost/mpi.hpp>
#endif

namespace SST {

    Sync::Sync(TimeConverter* period) : Action()
    {
        this->period = period;
        Simulation *sim = Simulation::getSimulation();
        SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
        setPriority(25);
        sim->insertActivity( next, this );
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
    
    SyncQueue* Sync::registerLink(int rank, LinkId_t link_id, Link* link)
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
            i->second->finalizeConfiguration();
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

        SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
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
                    link->second->sendInitData_sync(ev);
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
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
        printf("  - Sync::period\n");
        ar & BOOST_SERIALIZATION_NVP(period);
        printf("  - Sync::comm_map (%d)\n", (int) comm_map.size());
        ar & BOOST_SERIALIZATION_NVP(comm_map);
        printf("  - Sync::link_map (%d)\n", (int) link_map.size());
        ar & BOOST_SERIALIZATION_NVP(link_map);
        // don't serialize comm - let it be silently rebuilt at restart
        printf("end Sync::serialize\n");
    }
    
} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Sync::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Sync)
