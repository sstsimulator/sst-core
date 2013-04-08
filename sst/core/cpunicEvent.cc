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


#include "sst_config.h"
#include "sst/core/serialization/core.h"

#include "sst/core/cpunicEvent.h"

namespace SST {

template<class Archive>
void
CPUNicEvent::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
    ar & BOOST_SERIALIZATION_NVP(route);
    ar & BOOST_SERIALIZATION_NVP(reverse_route);
    ar & BOOST_SERIALIZATION_NVP(router_delay);
    ar & BOOST_SERIALIZATION_NVP(hops);
    ar & BOOST_SERIALIZATION_NVP(congestion_cnt);
    ar & BOOST_SERIALIZATION_NVP(congestion_delay);
    ar & BOOST_SERIALIZATION_NVP(local_traffic);
    ar & BOOST_SERIALIZATION_NVP(entry_port);
    ar & BOOST_SERIALIZATION_NVP(return_event);
    ar & BOOST_SERIALIZATION_NVP(dest);
    ar & BOOST_SERIALIZATION_NVP(msg_id);
    ar & BOOST_SERIALIZATION_NVP(msg_match_bits);
    ar & BOOST_SERIALIZATION_NVP(buf);
    ar & BOOST_SERIALIZATION_NVP(msg_len);
    ar & BOOST_SERIALIZATION_NVP(tag);

    ar & BOOST_SERIALIZATION_NVP(params_present);
    ar & BOOST_SERIALIZATION_NVP(routine);
    ar & BOOST_SERIALIZATION_NVP(params_len);
    ar & BOOST_SERIALIZATION_NVP(event_params);
    ar & BOOST_SERIALIZATION_NVP(msg_payload);
    ar & BOOST_SERIALIZATION_NVP(payload_present);
    ar & BOOST_SERIALIZATION_NVP(payload_len);
}

} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::CPUNicEvent::serialize)

BOOST_CLASS_EXPORT_IMPLEMENT(SST::CPUNicEvent)
