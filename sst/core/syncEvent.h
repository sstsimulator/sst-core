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


#ifndef _SST_SYNCEVENT_H
#define _SST_SYNCEVENT_H

#include <sst/core/event.h>

namespace SST {

class SyncEvent : public Event
{
public:
     SyncEvent( EventHandler_t* handler )
     {
       functor = handler;
     }

    SyncEvent() {}
    SyncEvent( const SyncEvent& e ) :
        Event( e ) 
    {}

private:
    EventHandler_t* functor;

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & boost::serialization::base_object<Event>(*this);
    }

    template<class Archive>
    friend void
    save_construct_data(Archive & ar, 
                        const SyncEvent * t, 
                        const unsigned int file_version)
    {
            EventHandler_t* handler = t->functor;
            ar << BOOST_SERIALIZATION_NVP( handler );
    }

    template<class Archive>
    friend void
    load_construct_data(Archive & ar, 
                        SyncEvent * t, 
                        const unsigned int file_version)
    {
        EventHandler_t* handler;
        ar >> BOOST_SERIALIZATION_NVP( handler );
        ::new(t)SyncEvent( handler );
    }
};

} // namespace SST

#endif
