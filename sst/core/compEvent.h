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

// This is a backward compatibility class and should not be used.
// Just make whatever you were thinking of making inherit from
// CompEvent inherit from Event instead.

#ifndef SST_COMPEVENT_H
#define SST_COMPEVENT_H

#include <sst/core/event.h>

namespace SST {

class CompEvent : public Event
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
    }
};

} // namespace SST

#endif // SST_COMPEVENT_H
