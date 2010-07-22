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



#ifndef _MEMEVENT_H
#define _MEMEVENT_H

#include <sst/core/event.h>

namespace SST {

class MemEvent : public Event {
    public:
        typedef enum { MEM_LOAD, MEM_LOAD_RESP, 
                MEM_STORE, MEM_STORE_RESP } Type_t;
        MemEvent() : Event() { }

        unsigned long   address;
        // this should be Type_t but SERIALIZATION barfs on it
        int             type;
        uint64_t        tag;
    
    private:
	
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
            ar & BOOST_SERIALIZATION_NVP( address );
            ar & BOOST_SERIALIZATION_NVP( type );
            ar & BOOST_SERIALIZATION_NVP( tag );
        }
};

    
} //namespace SST

#endif
