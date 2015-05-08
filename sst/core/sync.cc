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

#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/sync.h"

#include "sst/core/event.h"
#include "sst/core/exit.h"
#include "sst/core/link.h"
#include "sst/core/simulation.h"
#include "sst/core/syncQueue.h"
#include "sst/core/timeConverter.h"

#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>

#ifdef HAVE_MPI
#include <mpi.h>
#endif

namespace SST {

    void
    SyncBase::setMaxPeriod(TimeConverter* period)
    {
        max_period = period;
        Simulation *sim = Simulation::getSimulation();
        SimTime_t next = sim->getCurrentSimCycle() + period->getFactor();
        setPriority(SYNCPRIORITY);
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
    ////   SyncD
    
    SyncD::SyncD() :
        SyncBase()
    {
    }
    
    SyncD::~SyncD()
    {
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            delete i->second.squeue;
        }
        comm_map.clear();
        
        for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
            delete i->second;
        }
        link_map.clear();
    }
    
    ActivityQueue* SyncD::registerLink(int rank, LinkId_t link_id, Link* link)
    {
        SyncQueue* queue;
        if ( comm_map.count(rank) == 0 ) {
            queue = comm_map[rank].squeue = new SyncQueue();
            comm_map[rank].rbuf = new char[4096];
            comm_map[rank].local_size = 4096;
            comm_map[rank].remote_size = 4096;
        } else {
            queue = comm_map[rank].squeue;
        }
	
        link_map[link_id] = link;
        return queue;
    }

    void
    SyncD::finalizeLinkConfigurations() {
        for (link_map_t::iterator i = link_map.begin() ; i != link_map.end() ; ++i) {
            // i->second->finalizeConfiguration();
            finalizeConfiguration(i->second);
        }
    }

    uint64_t
    SyncD::getDataSize() const {
        size_t count = 0;
        for ( comm_map_t::const_iterator it = comm_map.begin();
              it != comm_map.end(); ++it ) {
            count += (it->second.squeue->getDataSize() + it->second.local_size);
        }
        return count;
    }
    
    void
    SyncD::execute(void)
    {
        // static Output tmp_debug("@r: @t:  ",5,-1,Output::FILE);
#ifdef HAVE_MPI
        // std::vector<boost::mpi::request> pending_requests;

        //Maximum number of outstanding requests is 3 times the number
        // of ranks I communicate with (1 recv, 2 sends per rank)
        MPI_Request sreqs[2 * comm_map.size()];
        MPI_Request rreqs[comm_map.size()];
        int sreq_count = 0;
        int rreq_count = 0;
        
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {

            // Do all the sends
            // Get the buffer from the syncQueue
            char* send_buffer = i->second.squeue->getData();
            // Cast to Header so we can get/fill in data
            SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
            int tag = 1;
            // Check to see if remote queue is big enough for data
            if ( i->second.remote_size < hdr->buffer_size ) {
                // not big enough, send message that will tell remote side to get larger buffer
                hdr->mode = 1;
                MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE,
                          i->first/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
                i->second.remote_size = hdr->buffer_size;
                tag = 2;
            }
            else {
                hdr->mode = 0;
            }
            MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE,
                      i->first/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);

            // Post all the receives
            MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE,
                      i->first, 1, MPI_COMM_WORLD, &rreqs[rreq_count++]);
        }

        // Wait for all sends and recvs to complete
        Simulation* sim = Simulation::getSimulation();
        SimTime_t current_cycle = sim->getCurrentSimCycle();

        MPI_Waitall(rreq_count, rreqs, MPI_STATUSES_IGNORE);

        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            // Get the buffer and deserialize all the events
            char* buffer = i->second.rbuf;

            SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(buffer);
//            int count = hdr->count;
            unsigned int size = hdr->buffer_size;
            int mode = hdr->mode;

            if ( mode == 1 ) {
                // May need to resize the buffer
                if ( size > i->second.local_size ) {
                    delete[] i->second.rbuf;
                    i->second.rbuf = new char[size];
                    i->second.local_size = size;
                }
                MPI_Recv(i->second.rbuf, i->second.local_size, MPI_BYTE,
                         i->first, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                buffer = i->second.rbuf;
            }
            
            boost::iostreams::basic_array_source<char> source(&buffer[sizeof(SyncQueue::Header)],size-sizeof(SyncQueue::Header));
            boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
            boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header | boost::archive::no_tracking );

            std::vector<Activity*> activities;
            ia >> activities;
            for ( unsigned int j = 0; j < activities.size(); j++ ) {

                Event* ev = static_cast<Event*>(activities[j]);
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
            
            // for ( int j = 0; j < count; j++ ) {
            //     Event* ev;
            //     ia >> ev;

            //     link_map_t::iterator link = link_map.find(ev->getLinkId());
            //     if (link == link_map.end()) {
            //         printf("Link not found in map!\n");
            //         abort();
            //     } else {
            //         // Need to figure out what the "delay" is for this event.
            //         SimTime_t delay = ev->getDeliveryTime() - current_cycle;
            //         link->second->send(delay,ev);
            //     }
            // }
        }

        // Clear the SyncQueues used to send the data after all the sends have completed
        MPI_Waitall(sreq_count, sreqs, MPI_STATUSES_IGNORE);
        
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            i->second.squeue->clear();
        }

        // If we have an Exit object, fire it to see if we need end simulation
        if ( exit != NULL ) exit->check();

        // Check to see when the next event is scheduled, then do an
        // all_reduce with min operator and set next sync time to be
        // min + max_period.
        // boost::mpi::communicator world;
        // SimTime_t input = Simulation::getSimulation()->getNextActivityTime();;
        // SimTime_t min_time;
        // all_reduce( world, &input, 1, &min_time, boost::mpi::minimum<SimTime_t>() );

        SimTime_t input = Simulation::getSimulation()->getNextActivityTime();;
        SimTime_t min_time;
        MPI_Allreduce( &input, &min_time, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD );

        // tmp_debug.output(CALL_INFO,"  my_time: %" PRIu64 ", min_time: %" PRIu64 "\n",input, min_time);
        
        SimTime_t next = min_time + max_period->getFactor();
        // std::cout << next << std::endl;
        sim->insertActivity( next, this );
#endif
    }

    int
    SyncD::exchangeLinkInitData(int msg_count)
    {
#ifdef HAVE_MPI

        //Maximum number of outstanding requests is 3 times the number
        // of ranks I communicate with (1 recv, 2 sends per rank)
        MPI_Request sreqs[2 * comm_map.size()];
        MPI_Request rreqs[comm_map.size()];
        int rreq_count = 0;
        int sreq_count = 0;
        
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {

            // Do all the sends
            // Get the buffer from the syncQueue
            char* send_buffer = i->second.squeue->getData();
            // Cast to Header so we can get/fill in data
            SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(send_buffer);
            int tag = 1;
            // Check to see if remote queue is big enough for data
            if ( i->second.remote_size < hdr->buffer_size ) {
                // std::cout << i->second.remote_size << ", " << hdr->buffer_size << std::endl;
                // not big enough, send message that will tell remote side to get larger buffer
                hdr->mode = 1;
                // std::cout << "MPI_Isend of header only to " << i->first << " with tag " << tag << std::endl;
                MPI_Isend(send_buffer, sizeof(SyncQueue::Header), MPI_BYTE, i->first/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);
                i->second.remote_size = hdr->buffer_size;
                tag = 2;
            }
            else {
                hdr->mode = 0;
            }
            // std::cout << "MPI_Isend of whole buffer to " << i->first << " with tag " << tag << std::endl;
            MPI_Isend(send_buffer, hdr->buffer_size, MPI_BYTE, i->first/*dest*/, tag, MPI_COMM_WORLD, &sreqs[sreq_count++]);

            // Post all the receives
            MPI_Irecv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first, 1, MPI_COMM_WORLD, &rreqs[rreq_count++]);
            
        }

        // Wait for all recvs to complete
        // std::cout << "Start waitall" << std::endl;
        MPI_Waitall(rreq_count, rreqs, MPI_STATUSES_IGNORE);
        // std::cout << "End waitall" << std::endl;


        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {

            // Get the buffer and deserialize all the events
            char* buffer = i->second.rbuf;

            SyncQueue::Header* hdr = reinterpret_cast<SyncQueue::Header*>(buffer);
//            int count = hdr->count;
            unsigned int size = hdr->buffer_size;
            int mode = hdr->mode;

            if ( mode == 1 ) {
                // May need to resize the buffer
                if ( size > i->second.local_size ) {
                    delete[] i->second.rbuf;
                    i->second.rbuf = new char[size];
                    i->second.local_size = size;
                }
                MPI_Recv(i->second.rbuf, i->second.local_size, MPI_BYTE, i->first, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                buffer = i->second.rbuf;
            }

            boost::iostreams::basic_array_source<char> source(&buffer[sizeof(SyncQueue::Header)],size-sizeof(SyncQueue::Header));
            boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
            boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header | boost::archive::no_tracking );

            std::vector<Activity*> activities;
            ia >> activities;
            for ( unsigned int j = 0; j < activities.size(); j++ ) {

                Event* ev = static_cast<Event*>(activities[j]);
                link_map_t::iterator link = link_map.find(ev->getLinkId());
                if (link == link_map.end()) {
                    printf("Link not found in map!\n");
                    abort();
                } else {
                    sendInitData_sync(link->second,ev);
                }
            }
            
            
            // for ( int j = 0; j < count; j++ ) {
            //     Event* ev;
            //     ia >> ev;
                
                
            //     link_map_t::iterator link = link_map.find(ev->getLinkId());
            //     if (link == link_map.end()) {
            //         printf("Link not found in map!\n");
            //         abort();
            //     } else {
            //         sendInitData_sync(link->second,ev);
            //     }
            // }

            // Clear the receive vector
            // tmp->clear();
        }

        // Clear the SyncQueues used to send the data after all the sends have completed
        MPI_Waitall(sreq_count, sreqs, MPI_STATUSES_IGNORE);
        
        for (comm_map_t::iterator i = comm_map.begin() ; i != comm_map.end() ; ++i) {
            i->second.squeue->clear();
        }

        // Do an allreduce to see if there were any messages sent
        // boost::mpi::communicator world;
        // int input = msg_count;
        // int count;
        // all_reduce( world, &input, 1, &count, std::plus<int>() );
        // return count;
        int input = msg_count;
        int count;
        MPI_Allreduce( &input, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
        return count;
#else
        return 0;
#endif
    }
    
    template<class Archive>
    void
    SyncD::serialize(Archive & ar, const unsigned int version)
    {
        // printf("begin Sync::serialize\n");
        // ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SyncBase);
        // printf("  - Sync::period\n");
        // // ar & BOOST_SERIALIZATION_NVP(period);
        // printf("  - Sync::comm_map (%d)\n", (int) comm_map.size());
        // ar & BOOST_SERIALIZATION_NVP(comm_map);
        // printf("  - Sync::link_map (%d)\n", (int) link_map.size());
        // ar & BOOST_SERIALIZATION_NVP(link_map);
        // // don't serialize comm - let it be silently rebuilt at restart
        // printf("end Sync::serialize\n");
    }
    
    
} // namespace SST
