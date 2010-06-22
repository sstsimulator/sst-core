// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization/core.h"
#include "sst/core/serialization/types.h"

#include "sst/core/sync.h"
#include "sst/core/syncQueue.h"
#include "sst/core/simulation.h"

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
    Sync::execute(void)
    {
        std::vector<boost::mpi::request> pending_requests;

        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            pending_requests.push_back(comm.isend(i->first, 0, i->second.first->getVector()));
            pending_requests.push_back(comm.irecv(i->first, 0, i->second.second));
        }
        boost::mpi::wait_all(pending_requests.begin(), pending_requests.end());

        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            i->second.first->clear();

            std::vector<Activity*> *tmp = i->second.second;
            for (std::vector<Activity*>::iterator j = tmp->begin() ; j != tmp->end() ; ++j) {
                Event *ev = static_cast<Event*>(*j);
                link_map_t::iterator link = link_map.find(ev->getLinkId());
                if (link == link_map.end()) {
                    printf("Link not found in map!\n");
                    abort();
                } else {
                    link->second->Send(ev);
                }
            }
            tmp->clear();
        }        

	Simulation *sim = Simulation::getSimulation();
	SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
	sim->insertActivity( next, this );
    }

} // namespace SST
