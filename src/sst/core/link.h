// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_LINK_H
#define SST_CORE_LINK_H

#include <sst/core/sst_types.h>

#include <sst/core/event.h>
// #include <sst/core/eventFunctor.h>

namespace SST { 

#define _LINK_DBG( fmt, args...) __DBG( DBG_LINK, Link, fmt, ## args )

class TimeConverter;
class LinkPair;
class Simulation;
class ActivityQueue;
class SyncBase;

class UnitAlgebra;
 
  /** Link between two components. Carries events */
class Link {
        typedef enum { POLL, HANDLER, QUEUE } Type_t;
public:

    friend class LinkPair;
    friend class NewRankSync;
    friend class NewThreadSync;
    friend class Simulation;
    friend class SyncBase;
    friend class ThreadSync;
    friend class SyncManager;
    
    /** Create a new link with a given ID */
    Link(LinkId_t id);
    
    virtual ~Link();
    
    /** set minimum link latency */
    void setLatency(Cycle_t lat);
    /** Set additional Latency to be added on to events coming in on this link.
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    void addRecvLatency(int cycles, std::string timebase);
    /** Set additional Latency to be added on to events coming in on this link.
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    void addRecvLatency(SimTime_t cycles, TimeConverter* timebase);

    /** Set the callback function to be called when a message is delivered. */
    void setFunctor(Event::HandlerBase* functor) {
        rFunctor = functor;
    }

    /** Specifies that this link has no callback, and is poll-based only */
    void setPolling();

    /** Send an event over the link with additional delay. Sends an event
      over a link with an additional delay specified with a
      TimeConverter. I.e. the total delay is the link's delay + the
      additional specified delay.
      @param delay - additional delay
      @param tc - time converter to specify units for the additional delay
      @param event - the Event to send
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
      @param event The event to send
    */
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
    /** Return the default Time Base for this link */
    TimeConverter* getDefaultTimeBase();

    /** Causes an event to be delivered to the registered callback */
    inline void deliverEvent(Event* event) const {
        (*rFunctor)(event);
    }

    /** Return the ID of this link */
    LinkId_t getId() { return id; }

    /** Send data during the init() phase. */
    void sendInitData(Event* init_data);
    /** Receive an event (if any) during the init() phase */
    Event* recvInitData();

#ifdef __SST_DEBUG_EVENT_TRACKING__
    void setSendingComponentInfo(const std::string& comp_in, const std::string& type_in, const std::string& port_in) {
        comp = comp_in;
        ctype = type_in;
        port = port_in;
    }

    const std::string& getSendingComponentName() { return comp; }
    const std::string& getSendingComponentType() { return ctype; }
    const std::string& getSendingPort() { return port; }

#endif
    // UnitAlgebra getTotalInputLatency();
    // UnitAlgebra getTotalOutputLatency();

protected:
    Link();

    /** Queue of events to be received by the owning component */
    ActivityQueue* recvQueue;
    /** Queue of events to be received during init by the owning component */
    ActivityQueue* initQueue;
    /** Currently active Queue */
    ActivityQueue* configuredQueue;
    /** Unitialized queue.  Used for error detection */
    static ActivityQueue* uninitQueue;
    /** Unitialized queue.  Used for error detection */
    static ActivityQueue* afterInitQueue;

    /** Recieve functor. This functor is set when the link is connected.
      Determines what the receiver wants to be called
    */
    Event::HandlerBase*  rFunctor;

    /** Timebase used if no other timebase is specified. Used to specify
      the untits for added delays when sending, such as in
      Link::send(). Often set by the Component::registerClock()
      function if the regAll argument is true.
      */
    TimeConverter*     defaultTimeBase;

    /** Latency of the link. It is used by the partitioner as the
      weight. This latency is added to the delay put on the event by
      the component.
    */
    SimTime_t            latency;

    /** Pointer to the opposite side of this link */
    Link* pair_link;

private:
    Link( const Link& l );

    void sendInitData_sync(Event* init_data);
    void finalizeConfiguration();
    
    Type_t type;
    LinkId_t id;

#ifdef __SST_DEBUG_EVENT_TRACKING__
    std::string comp;
    std::string ctype;
    std::string port;
#endif
    
    //    friend class SST::Sync;
    
};

/** Self Links are links from a component to itself */
class SelfLink : public Link {
public:
    SelfLink() : Link()
    {
        pair_link = this;
        latency = 0;
    }

};


} // namespace SST

#endif // SST_CORE_LINK_H
