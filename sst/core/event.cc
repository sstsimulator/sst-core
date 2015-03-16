// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/event.h"
#include "sst/core/simulation.h"

#include "sst/core/link.h"

namespace SST {


uint64_t SST::Event::id_counter = 0;
const SST::Event::id_type SST::Event::NO_ID = std::make_pair(0, -1);

Event::~Event() {}


void Event::execute(void)
{
    delivery_link->deliverEvent(this);
}

Event* Event::clone() {
    Simulation::getSimulation()->getSimulationOutput().
        fatal(CALL_INFO,1,"Called clone() on an Event that doesn't"
              " implement it.");            
    return NULL;  // Never reached, but gets rid of compiler warning
}

Event::id_type Event::generateUniqueId()
{
    return std::make_pair(id_counter++, Simulation::getSimulation()->getRank());
}


template<class Archive>
void Event::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Activity);
    // ar & BOOST_SERIALIZATION_NVP(delivery_link);
    ar & BOOST_SERIALIZATION_NVP(link_id);
}

void NullEvent::execute(void)
{
    delivery_link->deliverEvent(NULL);
    delete this;
}


template<class Archive>
void NullEvent::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
}


#ifdef USE_MEMPOOL
std::vector<std::pair<size_t, Core::MemPool*> > Activity::memPools;
#endif

} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Event::serialize)
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::NullEvent::serialize)

BOOST_CLASS_EXPORT_IMPLEMENT(SST::Event)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::NullEvent)
