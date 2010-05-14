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


#ifndef SST_EVENT_H
#define SST_EVENT_H

#include <sst/boost.h>
#include <sst/eventFunctor.h>

namespace SST {

class Event {
public:
    typedef EventHandlerBase<bool,Event*> Handler_t;

    Event() {}
    virtual ~Event() = 0;

private:
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
    }
};

}

#endif // SST_EVENT_H
