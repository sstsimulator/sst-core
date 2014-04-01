// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_LINK_H
#define SST_CORE_LINK_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <sst/core/event.h>
// #include <sst/core/eventFunctor.h>

namespace SST { 

#define _LINK_DBG( fmt, args...) __DBG( DBG_LINK, Link, fmt, ## args )

class TimeConverter;
class LinkPair;
class Simulation;
class ActivityQueue;
class Sync;

class UnitAlgebra;
 
  /** Link between two components. Carries events */
class Link {
        typedef enum { POLL, HANDLER, QUEUE } Type_t;
public:

    friend class LinkPair;
    friend class Simulation;
    friend class Sync;
    
    Link(LinkId_t id);
    
    virtual ~Link();
    
    /** set minimum link latency */
    void setLatency(Cycle_t lat);
    void addRecvLatency(int cycles, std::string timebase);
    void addRecvLatency(SimTime_t cycles, TimeConverter* timebase);
    
//     void setFunctor(EventHandler_t* functor) {
    void setFunctor(Event::HandlerBase* functor) {
	rFunctor = functor;
    }

    void setPolling();
    
    /** Send an event over the link with additional delay. Sends an event
	over a link with an additional delay specified with a
	TimeConverter. I.e. the total delay is the link's delay + the
	additional specified delay.
	@param delay The additional delay
	@param tc The time converter to specify units for the additional delay
	@param the Event to send
    */
    void send( SimTime_t delay, TimeConverter* tc, Event* event );   
    
    /** Send an event with additional delay. Sends an event over a link
	with additional delay specified by the Link's default
	timebase.
	@param delay The additional delay, in units of the default Link timebase
	@param event The event to send
    */
    inline void send( SimTime_t delay, Event* event ) {  
	send(delay,defaultTimeBase,event);
    }
    
    /** Send an event with the Link's default delay
	@param event The event to send */
    inline void send( Event* event ) {
	send( 0, event );
    }
    
    
    /** Retrieve a pending event from the Link. For links which do not
	have a set event handler, they can be polled with this function.
	Returns NULL if there is no pending event.
    */
    Event* recv();
    
    
    /** Manually set the default detaulTimeBase 
	@param tc TimeConverter object for the timebase */ 
    void setDefaultTimeBase(TimeConverter* tc);
    TimeConverter* getDefaultTimeBase();
    
    inline void deliverEvent(Event* event) {
	(*rFunctor)(event);
    }

    LinkId_t getId() { return id; }

    void sendInitData(Event* init_data);
    Event* recvInitData();

    // UnitAlgebra getTotalInputLatency();
    // UnitAlgebra getTotalOutputLatency();
    
protected:
    Link();

    ActivityQueue* recvQueue;
    ActivityQueue* initQueue;
    ActivityQueue* configuredQueue;
    static ActivityQueue* uninitQueue;
    static ActivityQueue* afterInitQueue;
    
    /** Recieve functor. This functor is set when the link is connected.
	Determines what the receiver wants to be called */ 
    Event::HandlerBase*  rFunctor;
    
    /** Timebase used if no other timebase is specified. Used to specify
	the untits for added delays when sending, such as in
	Link::send(). Often set by the Component::registerClock()
	function if the regAll argument is true. */
    TimeConverter*     defaultTimeBase;
    
    /** Latency of the link. It is used by the partitioner as the
	weight. This latency is added to the delay put on the event by
	the component. */
    SimTime_t            latency;

    Link* pair_link;

private:
    Link( const Link& l );

    void sendInitData_sync(Event* init_data);
    void finalizeConfiguration();
    
    Type_t type;
    LinkId_t id;
    
    //    friend class SST::Sync;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version );
};

class SelfLink : public Link {
public:
    SelfLink() :
	Link()
    {
	pair_link = this;
	latency = 0;
    }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version );
};    


} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Link)
BOOST_CLASS_EXPORT_KEY(SST::SelfLink)


#endif // SST_CORE_LINK_H
