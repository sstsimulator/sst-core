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


// This type of event is used to transmit an arbitrary chunk of
// data between a CPU and an attached NIC. I guess it could be
// used anywhere a chunk of data and length is sufficient.
//
// The idea is to pack netsim API info into a data structure and
// send it off to the NIC which will know what to do with the
// data.
//
#ifndef SST_CPUNICEVENT_H
#define SST_CPUNICEVENT_H

#include <cstring>

#include "sst/core/event.h"

namespace SST {

// We hardcode this here so we don't have to include netsim_internal.h
#define CPUNICEVNET_MAX_PARAMS		(64)

class CPUNicEvent : public Event {
    public:
        CPUNicEvent() : Event()   {
	    params_present= false;
	    params_len= 0;
	    routine= -1;
	    payload_len= 0;
	    payload_present= false;
	    hops= 0;
	}

	// How to route this event through the network
	std::vector<uint8_t>route;
	SimTime_t router_delay;
	int hops;

	// Some envelope info
	uint64_t msg_match_bits;
	uint64_t buf;
	uint32_t msg_len;

	// Functions to attach and detach parameters
	inline void AttachParams(const void *input, int len)   {
	    if (len > CPUNICEVNET_MAX_PARAMS)   {
		_ABORT(CPUNicEvent, "Only have room for %d bytes!!\n", CPUNICEVNET_MAX_PARAMS);
	    }
	    params_present= true;
	    params_len= len;
	    std::memcpy(event_params, input, len);
	}

	inline void DetachParams(void *output, int *len)   {
	    if (!params_present)   {
		_ABORT(CPUNicEvent, "No params present!\n");
	    }
	    if (*len > CPUNICEVNET_MAX_PARAMS)   {
		_ABORT(CPUNicEvent, "Can't detach %d bytes. Only have %d bytes (%d max) of params!!\n",
		    *len, params_len, CPUNICEVNET_MAX_PARAMS);
	    }
	    if ((int) params_len > *len)   {
		_ABORT(CPUNicEvent, "Have %d bytes of params, but user only wants %d!\n",
		    params_len, *len);
	    }

	    std::memcpy(output, event_params, params_len);
	    *len= params_len;
	}

	inline void SetRoutine(int r)   {
	    routine= r;
	}

	inline int GetRoutine(void)   {
	    return routine;
	}



	// Functions to attach and detach a message payload
	inline void AttachPayload(const char *payload, int len)   {
	    if (payload_present)   {
		_ABORT(NicEvent, "Payload data already present!\n");
	    }
	    payload_present= true;
	    msg_payload.reserve(len);
	    payload_len= len;
	    msg_payload.insert(msg_payload.end(), payload, payload + len);
	}

	inline void DetachPayload(void *output, int *len)   {
	    if (!payload_present)   {
		_ABORT(CPUNicEvent, "No payload present!\n");
	    }
	    if (*len > payload_len)   {
		_ABORT(CPUNicEvent, "Have %d bytes of payload, but user wants %d!\n",
		    payload_len, *len);
	    }

	    std::memcpy(output, &msg_payload[0], payload_len);
	    *len= payload_len;
	}

	inline int GetPayloadLen(void)   {
	    return payload_len;
	}


    private:
	bool params_present;
	int routine;
	unsigned int params_len;
	uint8_t event_params[CPUNICEVNET_MAX_PARAMS];
	std::vector<uint8_t>msg_payload;
	bool payload_present;
	int payload_len;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
	    ar & BOOST_SERIALIZATION_NVP(route);
	    ar & BOOST_SERIALIZATION_NVP(router_delay);
	    ar & BOOST_SERIALIZATION_NVP(hops);
	    ar & BOOST_SERIALIZATION_NVP(msg_match_bits);
	    ar & BOOST_SERIALIZATION_NVP(msg_len);
	    ar & BOOST_SERIALIZATION_NVP(event_params);
	    ar & BOOST_SERIALIZATION_NVP(routine);
	    ar & BOOST_SERIALIZATION_NVP(params_len);
	    ar & BOOST_SERIALIZATION_NVP(params_present);
	    ar & BOOST_SERIALIZATION_NVP(payload_len);
	    ar & BOOST_SERIALIZATION_NVP(msg_payload);
	    ar & BOOST_SERIALIZATION_NVP(payload_present);
        }
};
} //namespace SST

#endif // SST_CPUNICEVENT_H
