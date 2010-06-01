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



#ifndef _SST_LINK_H
#define _SST_LINK_H

#include <sst/core/sst.h>
#include <sst/core/compEvent.h>
#include <sst/core/eventQueue.h>

namespace SST { 

#define _LINK_DBG( fmt, args...) __DBG( DBG_LINK, Link, fmt, ## args )

class TimeConverter;
 class LinkPair;
 
  /** Link between two components. Carries events */
class Link {
        typedef enum { POLL, HANDLER, QUEUE } Type_t;
public:

    friend class LinkPair;
	
    Link( Event::Handler_t* functor );

    Link();
    
    ~Link();
    
    /** set minimum link latency */
    void setLatency(Cycle_t lat);

    void setFunctor(Event::Handler_t* functor) {
	rFunctor = functor;
    }

    void setPolling();
    
    void Connect( Link *link, Cycle_t lat );
    
    void Connect( CompEventQueue_t* queue, Cycle_t lat );
    
    /** Send an event over the link with additional delay. Sends an event
	over a link with an additional delay specified with a
	TimeConverter. I.e. the total delay is the link's delay + the
	additional specified delay.
	@param delay The additional delay
	@param tc The time converter to specify units for the additional delay
	@param the Event to send
    */
    void Send( SimTime_t delay, TimeConverter* tc, CompEvent* event );
    
    /** Send an event with additional delay. Sends an event over a link
	with additional delay specified by the Link's default
	timebase.
	@param delay The additional delay, in units of the default Link timebase
	@param event The event to send
    */
    inline void Send( SimTime_t delay, CompEvent* event ) {
	Send(delay,defaultTimeBase,event);
    }
    
    /** Send an event with the Link's default delay
	@param event The event to send */
    inline void Send( CompEvent* event ) {
	Send( 0, event );
    }
    
    void SyncInsert( Cycle_t cycle, CompEvent* event );
    
    /** Retrieve a pending event from the Link. For links which do not
	have a set event handler, they can be polled with this function.
	Returns NULL if there is no pending event.
    */
    CompEvent* Recv();
    
    void setSyncLink( Link* link, Event::Handler_t* functor ) {
        _LINK_DBG("Sync this=%p link=%p\n",this,link);
        m_syncLink = link;
	//         sFunctor = functor;
    }
    Event::Handler_t* RFunctor() {
        return rFunctor;
    }
    
    /** Manually set the default detaulTimeBase 
	@param tc TimeConverter object for the timebase */ 
    void setDefaultTimeBase(TimeConverter* tc);
    
protected:
    //     EventQueue_t*      sendQueue;
    EventQueue_t*      recvQueue;
    
    /** Recieve functor. This functor is set when the link is connected.
	Determines what the receiver wants to be called */ 
    Event::Handler_t*  rFunctor; 
    
    /** Send Functor. This get set during a connect on the send end of
	the link */ 
    //     Event::Handler_t*  sFunctor; 
    
    /** link to be used on far end of sync */
    Link*          m_syncLink;
    CompEventQueue_t*   m_syncQueue;
    
    /** Timebase used if no other timebase is specified. Used to specify
	the untits for added delays when sending, such as in
	Link::Send(). Often set by the Component::registerClock()
	function if the regAll argument is true. */
    TimeConverter*     defaultTimeBase;
    
    /** Latency of the link. It is used by the partitioner as the
	weight. This latency is added to the delay put on the event by
	the component. */
    SimTime_t            latency;
    
private:
    Link( const Link& l );
    
    Type_t type;
    
    Link* pair_link;
    LinkId_t id;
    // 	ActivityQueue* send_queue;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
	// 	    ar & BOOST_SERIALIZATION_NVP( sendQueue );
	ar & BOOST_SERIALIZATION_NVP( defaultTimeBase );
	ar & BOOST_SERIALIZATION_NVP( latency );
	ar & BOOST_SERIALIZATION_NVP( type );
    }
};
    
} // namespace SST

#endif
