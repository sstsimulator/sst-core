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


#ifndef SST_CORE_LINK_H
#define SST_CORE_LINK_H

#include <sst/core/sst_types.h>
// #include <sst/core/eventFunctor.h>
#include <sst/core/event.h>

namespace SST { 

#define _LINK_DBG( fmt, args...) __DBG( DBG_LINK, Link, fmt, ## args )

class TimeConverter;
class LinkPair;
class Simulation;
class ActivityQueue;
class Sync; 

// Class used to initialize links.  They can be sent in a component
// constructor and received in the setup() function.  The class
// extends Activity simply so we can use the exising infrastructure in
// Sync to transfer data between ranks.
class LinkInitData : public Activity {
private:
    std::string data_string;
    LinkId_t link_id;
    
public:
    LinkInitData() :
	Activity(),
	data_string(""),
	link_id(-1)
    {}
    
    LinkInitData(std::string init_data) :
	Activity(),
	data_string(init_data),
	link_id(-1)
    {
	std::cout << data_string << std::endl;
    }

    void execute() {}
    
    inline void setDataString(std::string str) {data_string = str;}
    inline std::string getDataString() {return data_string;}

    inline void setLinkId(LinkId_t id) {link_id = id;}
    inline LinkId_t getLinkId() {return link_id;}

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version );
};
 
  /** Link between two components. Carries events */
class Link {
        typedef enum { POLL, HANDLER, QUEUE } Type_t;
public:

    friend class LinkPair;
    friend class Simulation;

    Link(LinkId_t id);
    
    virtual ~Link();
    
    /** set minimum link latency */
    void setLatency(Cycle_t lat);
    void addOutputLatency(int cycles, std::string timebase);
    void addOutputLatency(SimTime_t cycles, TimeConverter* timebase);
    
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
    void Send( SimTime_t delay, TimeConverter* tc, Event* event );
    
    /** Send an event with additional delay. Sends an event over a link
	with additional delay specified by the Link's default
	timebase.
	@param delay The additional delay, in units of the default Link timebase
	@param event The event to send
    */
    inline void Send( SimTime_t delay, Event* event ) {
	Send(delay,defaultTimeBase,event);
    }
    
    /** Send an event with the Link's default delay
	@param event The event to send */
    inline void Send( Event* event ) {
	Send( 0, event );
    }
    
    
    /** Retrieve a pending event from the Link. For links which do not
	have a set event handler, they can be polled with this function.
	Returns NULL if there is no pending event.
    */
    Event* Recv();
    
    
    /** Manually set the default detaulTimeBase 
	@param tc TimeConverter object for the timebase */ 
    void setDefaultTimeBase(TimeConverter* tc);
    TimeConverter* getDefaultTimeBase();
    
    inline void deliverEvent(Event* event) {
	(*rFunctor)(event);
    }

    LinkId_t getId() { return id; }

    void sendInitData(LinkInitData* init_data);
    void sendInitData(std::string init_data);
    LinkInitData* recvInitData();
    std::string recvInitDataString();
    
protected:
    Link();

    ActivityQueue* recvQueue;
    
    /** Recieve functor. This functor is set when the link is connected.
	Determines what the receiver wants to be called */ 
    Event::HandlerBase*  rFunctor;
    
    /** Timebase used if no other timebase is specified. Used to specify
	the untits for added delays when sending, such as in
	Link::Send(). Often set by the Component::registerClock()
	function if the regAll argument is true. */
    TimeConverter*     defaultTimeBase;
    
    /** Latency of the link. It is used by the partitioner as the
	weight. This latency is added to the delay put on the event by
	the component. */
    SimTime_t            latency;

    Link* pair_link;

    LinkInitData* initData;

private:
    Link( const Link& l );
    
    Type_t type;
    LinkId_t id;
    
    friend class SST::Sync;
    void moveInitDataToRecvQueue();
    
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
BOOST_CLASS_EXPORT_KEY(SST::LinkInitData)


#endif
