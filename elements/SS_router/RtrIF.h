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


// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef ED_RTRIF_H
#define ED_RTRIF_H

#include <sstream>

#include <deque>

#include <sst/sst.h>
#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include <sst/log.h>
#include <paramUtil.h>
#include "SS_network.h"

#define RTRIF_DBG 1 
#ifndef RTRIF_DBG
#define RTRIF_DBG 0
#endif

#define db_RtrIF(fmt,args...) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

namespace SST {

class RtrIF : public Component {

private:

    int rtrCountP;

    typedef std::deque<RtrEvent*> ToNic;

    class ToRtr {

        int tokensP;
        deque<RtrEvent*> &eventQP;

    public:
        ToRtr( int num_tokens, deque<RtrEvent*> &eventQ ) :
                tokensP(num_tokens), eventQP(eventQ) {}

        bool push( RtrEvent* event) {
            networkPacket* pkt = &event->u.packet;
            if ( pkt->sizeInFlits() > tokensP ) return false;
            tokensP -= pkt->sizeInFlits();
            eventQP.push_back(event);
            return true;
        }
        int size() {
            return eventQP.size();
        }

        bool willTake( int numFlits ) {
            return (numFlits <= tokensP );
        }

        void returnTokens( int num ) {
            tokensP += num;
        }
    };

    map<int,ToNic*>         toNicMapP;
    map<int,ToRtr*>         toRtrMapP;

    uint                    num_vcP;

    deque<RtrEvent*>        toRtrQP;

    Link*                   m_rtrLink;
    Log< RTRIF_DBG >&       m_dbg;
    Log< RTRIF_DBG >&       m_dummyDbg;
    Log<>&                  m_log;


protected:
    int                     m_id;
    std::string             frequency;
    
public:
    int Finish() {
      return 0;
	}

   RtrIF( ComponentId_t id, Params_t& params ) :
        Component(id),
        num_vcP(2),
        rtrCountP(0),
        m_dbg( *new Log< RTRIF_DBG >( "RtrIF::", false ) ),
        m_dummyDbg( *new Log< RTRIF_DBG >( "Dummy::RtrIF::", false ) ),
        m_log( *new Log<>( "INFO RtrIF: ", false ) )
    {
        int num_tokens = 512;

        if ( params.find( "info" ) != params.end() ) {
            if ( params[ "info" ].compare( "yes" ) == 0 ) {
                m_log.enable();
            }
        }

        if ( params.find( "debug" ) != params.end() ) {
            if ( params[ "debug" ].compare( "yes" ) == 0 ) {
                m_dbg.enable();
            }
        }

        if ( params.find( "dummyDebug" ) != params.end() ) {
            if ( params[ "dummyDebug" ].compare( "yes" ) == 0 ) {
                m_dummyDbg.enable();
            }
        }

        if ( params.find( "id" ) == params.end() ) {
            _abort(RtrIF,"couldn't find routerID\n" );
        }
        m_id = str2long( params[ "id" ] );

        if ( params.find("clock") != params.end() ) {
            frequency = params["clock"];
        }

        if ( params.find( "num_vc" ) != params.end() ) {
            num_vcP = str2long( params["num_vc"] );
        }

        if ( params.find( "Node2RouterQSize_flits" ) != params.end() ) {
            num_tokens = str2long( params["Node2RouterQSize_flits"] );
        }

        std::ostringstream idStr;
        idStr << m_id << ":";
        m_dbg.prepend( idStr.str() );
        m_dummyDbg.prepend( idStr.str() );
        m_log.prepend( idStr.str() );

        m_log.write("num_vc=%d num_tokens=%d\n",num_vcP,num_tokens);
        m_log.write("nic id=%d frequency=%s\n", m_id, frequency.c_str());

        Event::Handler_t*   handler = new EventHandler<
                            RtrIF, bool, Event* >
                       ( this, &RtrIF::processEvent );

        m_rtrLink = LinkAdd( "rtr", handler );
	
/*         handler = new EventHandler< */
/*                             RtrIF, bool, Event* > */
/*                        ( this, &RtrIF::processCPUEvent ); */

/*         m_cpuLink = LinkAdd( "cpu", handler ); */
	
/*         m_selfLink = selfLink( "self"); */

/*         Params_t tmp_params; */
/*         findParams( "dummy.", params, tmp_params ); */
/*         dummyInit( tmp_params, frequency ); */

        ClockHandler_t* clockHandler = new EventHandler< RtrIF, bool, Cycle_t >
                                                ( this, &RtrIF::clock );

        if ( ! registerClock( frequency, clockHandler ) ) {
            _abort(XbarV2,"couldn't register clock handler");
        }

        db_RtrIF("Done registering clock\n");

        for ( int i=0; i < num_vcP; i++ ) {
            toNicMapP[i] = new ToNic();
            toRtrMapP[i] = new ToRtr(num_tokens,toRtrQP);
        }
/* 	printf("Finish construction RtrIF\n"); */

    }

    bool toNicQ_empty(int vc)
    {
        if ( vc >= num_vcP ) _abort(RtrIF,"vc=%d\n",vc);
        return toNicMapP[vc]->empty();
    }

    RtrEvent *toNicQ_front(int vc)
    {
        if ( vc >= num_vcP ) _abort(RtrIF,"vc=%d\n",vc);
        db_RtrIF("vc=%d\n",vc);
        return toNicMapP[vc]->front();
    }

    void toNicQ_pop(int vc)
    {
        if ( vc >= num_vcP ) _abort(RtrIF,"vc=%d\n",vc);
        db_RtrIF("vc=%d\n",vc);
        returnTokens2Rtr( vc, toNicMapP[vc]->front()->u.packet.sizeInFlits() );
        toNicMapP[vc]->pop_front();
    }

    bool send2Rtr( RtrEvent *event)
    {
/*       printf("In send2Rtr\n"); */
      networkPacket* pkt = &event->u.packet;
        if ( pkt->vc() >= num_vcP ) _abort(RtrIF,"vc=%d\n",pkt->vc());
        bool retval = toRtrMapP[pkt->vc()]->push( event );
        if ( retval )
            db_RtrIF("vc=%d src=%d dest=%d pkt=%p\n",
                     pkt->vc(),pkt->srcNum(),pkt->destNum(),pkt);
        if ( retval ) {
            sendPktToRtr( toRtrQP.front() );
            toRtrQP.pop_front();
        }
        return retval;
    }

private:

//     void dummyInit( Params_t, std::string );
//     bool dummyLoad( Cycle_t cycle );
//     int   m_dummyFd_in;
//     int   m_dummyFd_out;
//     size_t  m_dummySize;
//     size_t  m_dummyOffset;

    bool rtrWillTake( int vc, int numFlits )
    {
        if ( vc >= num_vcP ) _abort(RtrIF,"\n");
        db_RtrIF("vc=%d numFlits=%d\n",vc,numFlits);
        return toRtrMapP[vc]->willTake( numFlits );
    }


//     bool processCPUEvent( Event* e)
//     {
//       // Just put event into self link and add the latency
//       // through the NIC to it
// /*       printf("Got an event from the cpu, forwarding it on\n"); */
// 	// If this is a portals event, then we add the latency
// 	// specified in the trig_nic_event
// 	trig_nic_event* ev = static_cast<trig_nic_event*>(e);
// 	m_selfLink->Send(ev->portals ? ev->latency : m_latency,ev);
//       return false;
//     }

    
    bool processEvent( Event* e)
    {
        RtrEvent* event = static_cast<RtrEvent*>(e);

/* 	printf("In processEvent()\n"); */
        db_RtrIF("type=%ld\n",event->type);

        switch ( event->type ) {
        case RtrEvent::Credit:
            returnTokens2Nic( event->u.credit.vc, event->u.credit.num );
            delete event;
            break;

        case RtrEvent::Packet:
            send2Nic( event );
            break;

        default:
            _abort(RtrIF,"unknown type %d\n",event->type);
        }
        return false;
    }

    bool clock( Cycle_t cycle)
    {
        rtrCountP = (rtrCountP >= 0) ? 0 : rtrCountP + 1;

        if ( ! toRtrQP.empty() ) {
            sendPktToRtr( toRtrQP.front());
            toRtrQP.pop_front();
        }
        return false;
    }

    void send2Nic( RtrEvent* event )
    {
        networkPacket *pkt = &event->u.packet; 

        pkt->vc() = RTR_2_NIC_VC(pkt->vc());

        if ( pkt->vc() >= num_vcP ) {
            _abort(RtrIF,"vc=%d pkt=%p\n",pkt->vc(),pkt);
        }

        db_RtrIF("vc=%d src=%d dest=%d pkt=%p\n",
                 pkt->vc(),pkt->srcNum(),pkt->destNum(),pkt);
        toNicMapP[pkt->vc()]->push_back( event );
    }

    void returnTokens2Nic( int vc, uint32_t num )
    {
        if ( vc >= num_vcP ) _abort(RtrIF,"\n");
        db_RtrIF("vc=%d numFlits=%d\n", vc, num );
        toRtrMapP[vc]->returnTokens( num );
    }

    void returnTokens2Rtr( int vc, uint numFlits ) 
    {
        RtrEvent* event = new RtrEvent;

        db_RtrIF("vc=%d numFlits=%d\n", vc, numFlits );

        event->type = RtrEvent::Credit;
        event->u.credit.num = numFlits;
        event->u.credit.vc = vc;
        m_rtrLink->Send( event );
    }

    void sendPktToRtr( RtrEvent* event ) 
    {
        networkPacket* pkt = &event->u.packet;

        db_RtrIF("vc=%d src=%d dest=%d pkt=%p\n",
                 pkt->vc(),pkt->srcNum(),pkt->destNum(),pkt);

        event->type = RtrEvent::Packet;
        event->u.packet = *pkt;
        int lat = reserveRtrLine(pkt->sizeInFlits());
        m_rtrLink->Send( lat, event );
    }

    int reserveRtrLine (int cyc)
    {
        db_RtrIF("cyc=%d\n",cyc);
        int ret = (rtrCountP <= 0) ? -rtrCountP : 0;
        rtrCountP -= cyc;
        return ret;
    }
};
}
#endif
