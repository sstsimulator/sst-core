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


#include <sst_config.h>

#include <sst/core/sync.h>
#include <sst/core/syncEvent.h>
#include <sst/core/link.h>
#include <sst/core/simulation.h>

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                SST::Sync, bool, SST::Event* )
#endif

enum { SyncMsgTag = 0x1, SyncExchangeMsgTag }; 

typedef struct {
    unsigned long data[2];
} functor_t;

struct functorMsg {
    static const int nameLen = 256;
    char                linkName[nameLen+1];
    unsigned long       linkPtr;
    functor_t           functor;

    friend class boost::serialization::access;
    template<class Archive> 
    void serialize(Archive & ar, const unsigned int version )
    {
        for( int i = 0; i < nameLen; i++ ) { 
            ar & BOOST_SERIALIZATION_NVP( linkName[i] );
        }
        ar & BOOST_SERIALIZATION_NVP( linkPtr );
        ar & BOOST_SERIALIZATION_NVP( functor.data[0] );
        ar & BOOST_SERIALIZATION_NVP( functor.data[1] );
    }
};

struct recvEntry {
    boost::mpi::request req;
    functorMsg          msg;
};

BOOST_IS_MPI_DATATYPE( functorMsg )

namespace SST {

Sync::Sync( TimeConverter* period ) :
    m_functor( new EventHandler<Sync,bool,Event*> (this,&Sync::handler)),
    m_period( period )
{
    _SYNC_DBG( "epoch %lu sim cyles \n", period->getFactor() );
    m_event = new SyncEvent( m_functor );
    Simulation::getSimulation()->insertEvent( m_period->getFactor(), m_event, m_functor );
}

int Sync::registerLink( std::string linkName, Link* link, 
                                        int rank, Cycle_t lat )
{
    _SYNC_DBG( "farRank=%d linkName=%s localLink=%p\n",
                    rank, linkName.c_str(), link );

    if ( m_linkMap.find( linkName) != m_linkMap.end() ) {
        _abort( Sync, "linkName \"%s\" is already present\n", linkName.c_str());
    }

    if ( linkName.length() > (unsigned int) functorMsg::nameLen ) {
        _abort( Sync, "linkName \"%s\" is too long, max length is %d\n",
                        linkName.c_str(), functorMsg::nameLen );
    }

    if ( m_rankMap.find( rank ) == m_rankMap.end() ) {
        boost::mpi::communicator            world;
        m_rankMap[rank].sendQ = new CompEventQueue_t;
        m_rankMap[rank].recvQ = new CompEventQueue_t;
        m_rankMap[rank].mpiRecvReq = 
                    world.irecv( rank, SyncMsgTag, *m_rankMap[rank].recvQ );
    }

    link->Connect( m_rankMap[rank].sendQ, lat );

    m_linkMap[linkName].link = link;
    m_linkMap[linkName].rank = rank;

    return 0;
}

int Sync::exchangeFunctors()
{
    boost::mpi::communicator    world;
    std::vector< recvEntry >    recvV;

    recvV.resize( m_linkMap.size() );

    for ( unsigned int i = 0; i < recvV.size(); i++ ) 
    {
        recvV[i].req = world.irecv( boost::mpi::any_source,
                            SyncExchangeMsgTag, recvV[i].msg );
    }

    world.barrier();
    
    for ( std::map<std::string,linkInfo>::iterator iter = m_linkMap.begin();
          iter != m_linkMap.end(); 
          ++iter
        ) 
    {
        functorMsg msg;
        strcpy( msg.linkName, (*iter).first.c_str() );
        msg.linkPtr = (unsigned long) (*iter).second.link;

        Event::Handler_t* functor = (*iter).second.link->RFunctor();

        functor_t* functor_ptr = reinterpret_cast<functor_t*>( &functor );
        msg.functor.data[0] = functor_ptr->data[0];  
        msg.functor.data[1] = functor_ptr->data[1];  

        _SYNC_DBG("send(%d), linkName=%s link=%#lx functor=%#lx:%#lx\n",
                        (*iter).second.rank, msg.linkName, msg.linkPtr, 
                        msg.functor.data[0], msg.functor.data[1] );
        world.send( (*iter).second.rank, SyncExchangeMsgTag, msg );
    }

    for ( unsigned int i = 0; i < recvV.size(); i++ ) 
    {
        recvV[i].req.wait();

        functorMsg&  msg  = recvV[i].msg;
        _SYNC_DBG( "got name=%s link=%#lx functor=%#lx:%#lx\n",
                           msg.linkName, msg.linkPtr,
                           msg.functor.data[0], msg.functor.data[1] );

        if ( m_linkMap.find( msg.linkName ) == m_linkMap.end() ) {
            _abort( Sync, "can't find linkName \"%s\"\n", msg.linkName );
        }

        linkInfo&    info = m_linkMap[ msg.linkName ]; 

        Event::Handler_t* functor;
        functor_t* functor_ptr = reinterpret_cast<functor_t*>( &functor );

        functor_ptr->data[0] = msg.functor.data[0];
        functor_ptr->data[1] = msg.functor.data[1];

        info.link->setSyncLink( (Link*) msg.linkPtr, functor );
    }
    return 0;
}


bool Sync::handler( Event *e )
{
    Simulation *sim = Simulation::getSimulation();
    SimTime_t next = sim->getCurrentSimCycle() + m_period->getFactor();

    _SYNC_DBG( "next cycle %lu\n", next );
    sim->insertEvent( next, m_event, m_functor );

    boost::mpi::communicator world;

    for ( std::map<int, rankInfo >::iterator iter = m_rankMap.begin();
          iter != m_rankMap.end();
          ++iter ) 
    {
        int rank = (*iter).first;
        rankInfo& info = (*iter).second;

        if ( ! info.sendQ->empty() )  
            _SYNC_DBG( "sending %ld events to rank %d\n", 
                                            info.sendQ->size(), rank );
       
        info.mpiSendReq = world.isend( rank, SyncMsgTag, *info.sendQ );
    }
        
    for ( std::map<int, rankInfo >::iterator iter = m_rankMap.begin();
          iter != m_rankMap.end();
          ++iter ) 
    {
        int rank = (*iter).first;
        rankInfo& info = (*iter).second;
        
        info.mpiRecvReq.wait();

        if ( ! info.recvQ->empty() )
            _SYNC_DBG( "got %ld events from rank %d\n", 
                                    info.recvQ->size(), rank );

        while ( ! info.recvQ->empty() ) {
            CompEvent* event = info.recvQ->front();

            _SYNC_DBG("destLink=%p cycle=%lu\n", event->LinkPtr(),
                                            event->Cycle() );

            event->LinkPtr()->SyncInsert( event->Cycle(), 
                                    static_cast< CompEvent* >( event ) );
            info.recvQ->pop_front();
        }
    }

    for ( std::map<int, rankInfo >::iterator iter = m_rankMap.begin();
          iter != m_rankMap.end();
          ++iter ) 
    {
        int rank = (*iter).first;
        rankInfo& info = (*iter).second;

        info.sendQ->clear();
    
        info.mpiRecvReq = world.irecv( rank, SyncMsgTag, *info.recvQ );
    }

    // could we get get rid of this barrier if we use 2 receive buffers and 
    // flip between them? plus we don't want to barrier with ranks 
    // that we are not syncing with
    world.barrier();

    return false;
}

} // namespace  SST

