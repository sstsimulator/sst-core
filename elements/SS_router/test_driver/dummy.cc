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

#include <sys/stat.h>
#include <fcntl.h>

#include "RtrIF.h"

#define db_dummy(fmt,args...) \
    m_dummyDbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)


void RtrIF::dummyInit( Params_t params, std::string frequency )
{
    registerExit();
    ClockHandler_t*  clockHandler = new EventHandler< RtrIF, bool, Cycle_t >
                                                ( this, &RtrIF::dummyLoad );

    if ( ! registerClock( frequency, clockHandler ) ) {
        _abort(XbarV2,"couldn't register clock handler");
    }
 
    if ( params.find("nodes") == params.end() ) {
	_abort(RtrIF,"couldn't find number of nodes\n");
    }
    m_num_nodes = str2long( params[ "nodes" ] );

    if ( params.find( "file" ) == params.end() ) {
        _abort(RtrIF,"couldn't find file\n" );
    }

//     std::string file = params["file"];
//     db_dummy( "%s\n", file.c_str() );

//     m_dummyFd_in = open(file.c_str(), O_RDONLY );
//     if ( m_dummyFd_in == -1 ) {
//         _abort( RtrIF," couldn't open file \"%s\"\n", file.c_str() );
//     }

//     struct stat buf;
//     if ( fstat( m_dummyFd_in, &buf ) == -1 ) {
//         _abort( RtrIF," couldn't stat file \"%s\"\n", file.c_str() );
//     }
//     m_dummySize = buf.st_size;
//     m_dummyOffset = 0;


//     file = file + ".out";
//     m_dummyFd_out = open(file.c_str(), O_CREAT|O_WRONLY, 0777 );
//     if ( m_dummyFd_out == -1 ) {
//         _abort( RtrIF," couldn't open file \"%s\"\n", file.c_str() );
//     }

    m_current_send_node = 0;
    m_node_recvd = 1;
    m_exit = false;
}

static inline int pktSize()
{
    return PKT_SIZE * sizeof(uint32);
}

bool::RtrIF::dummyLoad ( Cycle_t cycle ) {
    if ( !toNicQ_empty( 0 ) ) {
	RtrEvent* event = toNicQ_front( 0 );
	toNicQ_pop( 0 );
	//	int send_time = event->u.packet.payload[0];
	//	printf("latency (%d -> %d): %d\n",event->u.packet.srcNum(),m_id,getCurrentSimTimeNano()-send_time);
	delete event;
// 	printf("%d: received event from: %d\n",m_id,event->u.packet.srcNum());
	m_node_recvd++;
    }

//     if ( cycle % (m_id+1) != 0 ) return false;
//     if ( cycle % 32 != m_id % 32 ) return false;
//     if ( cycle % 32 != 0 ) return false;

    // Send a short message to each of the other nodes in the sim
    for ( ; m_current_send_node < m_num_nodes; m_current_send_node++ ) {
	if ( m_current_send_node == m_id ) {
	    m_current_send_node++;
	    break;
	}
	RtrEvent* event = new RtrEvent();
	event->type = RtrEvent::Packet;
	event->u.packet.vc() = 0;
	event->u.packet.srcNum() = m_id;
	event->u.packet.destNum() = m_current_send_node;
	event->u.packet.sizeInFlits() = 8;
// 	event->u.packet.payload[0] = m_id;
 	event->u.packet.payload[0] = getCurrentSimTimeNano();

	if ( !send2Rtr(event) ) {
	    delete event;
	    break;
	}
	if ( m_id == 0 )
	    printf("%d: Sending event to node %d on cycle %lu\n",m_id,m_current_send_node,cycle);
	m_current_send_node++;
	break;  // Only send one per cycle
    }

    if ( !m_exit && m_node_recvd >= (m_num_nodes) && m_current_send_node >= m_num_nodes ) {
	printf("%d: Unregistering exit on cycle %lu\n",m_id,cycle);
	m_exit = true;
        unregisterExit();
    }
    
    return false;
}


/*
bool RtrIF::dummyLoad( Cycle_t cycle )
{
    if ( m_dummyOffset >= m_dummySize ) {
        return false;
    }

    if ( m_id == 0 ) {

        db_dummy("send event to router\n");
        
        static RtrEvent* event = NULL;
        if ( event == NULL ) { 
            event = new RtrEvent;
            event->type = RtrEvent::Packet;
            event->u.packet.vc() = 0;
            event->u.packet.srcNum() = 0;
            event->u.packet.destNum() = 1;

            size_t size = m_dummyOffset + pktSize() ? pktSize() : 
                                        m_dummySize - m_dummyOffset; 

            event->u.packet.sizeInFlits() = CalcNumFlits( size / 4 );

            read( m_dummyFd_in, event->u.packet.payload, size );
        }

        if ( send2Rtr( event ) ) {
            db_dummy("send2Rtr success\n");
            event = NULL;
            m_dummyOffset += pktSize();
        }
    }

    if ( m_id == 1 ) {
        while ( ! toNicQ_empty( 0 ) ) {
            db_dummy("got event from router\n");
            RtrEvent* event = toNicQ_front( 0 );
        
            m_dummyOffset += pktSize();
            write( m_dummyFd_out, event->u.packet.payload, pktSize() );
            toNicQ_pop( 0 );
            delete event;
        }
    }
    //db_dummy("%ld %ld\n",m_dummySize,m_dummyOffset);

    if ( m_dummyOffset >= m_dummySize ) {
        unregisterExit();
    }

    return false;
}
*/
