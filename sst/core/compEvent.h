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


#ifndef SST_COMPEVENT_H
#define SST_COMPEVENT_H

#include <deque>

#include <sst/core/event.h>

namespace SST {

#define _CE_DBG( fmt, args...) __DBG( DBG_COMPEVENT, CompEvent, fmt, ## args )

class Link;

class CompEvent : public Event
{
public:
    CompEvent() {}

    void SetCycle( Cycle_t _cycle ) { cycle = _cycle; }
    void SetLinkPtr( Link* link ) { linkPtr = (unsigned long) link; }
    SimTime_t Cycle() { return cycle; }
    Link* LinkPtr() { return (Link*) linkPtr; }

private:
    CompEvent( const CompEvent& e );

    SimTime_t       cycle; 
    // used by Sync object cast to Link* by receiving Sync object
    unsigned long   linkPtr;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        boost::serialization::base_object<Event>(*this);
        ar & BOOST_SERIALIZATION_NVP( cycle );
        ar & BOOST_SERIALIZATION_NVP( linkPtr );
    }
};

typedef std::deque<CompEvent*>      CompEventQueue_t;

} // namespace SST

#endif // SST_COMPEVENT_H
