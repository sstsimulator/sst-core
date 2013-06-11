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

#ifndef _MEMEVENT_H
#define _MEMEVENT_H

#include <sst/core/sst_types.h>
#include <sst/core/debug.h>
#include <sst/core/activity.h>

namespace SST {
namespace Interfaces {


typedef uint64_t Addr;

#define X_TYPES \
	/* CPU <-> Cache */ \
	X(ReadReq) \
	X(ReadResp) \
	X(WriteReq) \
	X(WriteResp) \
	/* Cache <-> Cache/MemControl/DirCtrl */ \
	X(RequestData) \
	X(SupplyData) \
	X(Invalidate) \
    X(ACK) \
    X(NACK) \
	/* Component <-> Bus */ \
	X(RequestBus) \
	X(CancelBusRequest) \
	X(BusClearToSend) \
    /* Directory Controller */ \
    X(Fetch) \
    X(FetchInvalidate) \
	X(NULLCMD)

typedef enum {
#define X(x) x,
	X_TYPES
#undef X
} Command;

//extern const char* CommandString[];
static const char* CommandString[] __attribute__((unused)) = {
#define X(x) #x ,
	X_TYPES
#undef X
};

#undef X_TYPES


static const std::string BROADCAST_TARGET = "BROADCAST";

class MemEvent : public SST::Event {
public:
	static const uint32_t F_WRITEBACK = (1<<0);
	static const uint32_t F_LOCKED    = (1<<1);
    static const uint32_t F_DELAYED   = (1<<2); // Used to delay snoops when a block is locked
    static const uint32_t F_EXCLUSIVE = (1<<3);

	typedef std::vector<uint8_t> dataVec;
	typedef std::pair<uint64_t, int> id_type;

	MemEvent(const Component *_src, Addr _addr, Command _cmd) :
		SST::Event(), addr(_addr), cmd(_cmd), src(_src->getName())
	{
		event_id = std::make_pair(main_id++, _src->getId());
		response_to_id = std::make_pair(0, -1);
		dst = BROADCAST_TARGET;
		size = 0;
		flags = 0;
	}

	MemEvent(const MemEvent &me) : SST::Event()
	{
		event_id = me.event_id;
		response_to_id = me.response_to_id;
		addr = me.addr;
		size = me.size;
		cmd = me.cmd;
		payload = me.payload;
		src = me.src;
		dst = me.dst;
		flags = me.flags;
		setDeliveryLink(me.getLinkId(), NULL);
	}
	MemEvent(const MemEvent *me) : SST::Event()
	{
		event_id = me->event_id;
		response_to_id = me->response_to_id;
		addr = me->addr;
		size = me->size;
		cmd = me->cmd;
		payload = me->payload;
		src = me->src;
		dst = me->dst;
		flags = me->flags;
		setDeliveryLink(me->getLinkId(), NULL);
	}


	MemEvent* makeResponse(const Component *source)
	{
		MemEvent *me = new MemEvent(source, addr, commandResponse(cmd));
		me->setSize(size);
		me->response_to_id = event_id;
		me->dst = src;
		return me;
	}

	id_type getID(void) const { return event_id; }
	id_type getResponseToID(void) const { return response_to_id; }
	Command getCmd(void) const { return cmd; }
	void setCmd(Command newcmd) { cmd = newcmd; }
	Addr getAddr(void) const { return addr; }
	void setAddr(Addr newAddr) { addr = newAddr; }

	uint32_t getSize(void) const { return size; }
	void setSize(uint32_t sz) {
		size = sz;
	}

	dataVec& getPayload(void)
    {
        /* Lazily allocate space for payload */
        if ( payload.size() < size )
            payload.resize(size);
        return payload;
    }
	void setPayload(uint32_t size, uint8_t *data)
    {
		setSize(size);
        payload.resize(size);
		for ( uint32_t i = 0 ; i < size ; i++ ) {
			payload[i] = data[i];
		}
	}
	void setPayload(std::vector<uint8_t> &data) {
		setSize(data.size());
		payload = data;
	}

	const std::string& getSrc(void) const { return src; }
	void setSrc(const std::string &s) { src = s; }
	const std::string& getDst(void) const { return dst; }
	void setDst(const std::string &d) { dst = d; }

	bool isBroadcast(void) const { return (dst == BROADCAST_TARGET); }

	uint32_t getFlags(void) const { return flags; }
	void setFlag(uint32_t flag) { flags = flags | flag; }
	void clearFlag(uint32_t flag) { flags = flags & (~flag); }
	void clearFlags(void) { flags = 0; }
	bool queryFlag(uint32_t flag) const { return flags & flag; };
	void setFlags(uint32_t _flags) { flags = _flags; }
	uint64_t getLockID(void) const { return lockid; }
	void setLockID(uint64_t id) { lockid = id; }

private:
	static uint64_t main_id;
	id_type event_id;
	id_type response_to_id;
	uint64_t lockid; // Key for what thread is locking.
	Addr addr;
	uint32_t size;
	Command cmd;
	dataVec payload;
	std::string src;
	std::string dst;
	uint32_t flags;


	MemEvent() {} // For serialization only

	Command commandResponse(Command c)
	{
		switch(c) {
		case RequestData:
			return SupplyData;
		case SupplyData:
			return WriteResp;
		case ReadReq:
			return ReadResp;
		case WriteReq:
			return WriteResp;
        case Invalidate:
            return ACK;
        case Fetch:
        case FetchInvalidate:
            return SupplyData;
		default:
			return NULLCMD;
		}
	}

	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
		ar & BOOST_SERIALIZATION_NVP(event_id);
		ar & BOOST_SERIALIZATION_NVP(response_to_id);
		ar & BOOST_SERIALIZATION_NVP(addr);
		ar & BOOST_SERIALIZATION_NVP(size);
		ar & BOOST_SERIALIZATION_NVP(cmd);
		ar & BOOST_SERIALIZATION_NVP(payload);
		ar & BOOST_SERIALIZATION_NVP(src);
		ar & BOOST_SERIALIZATION_NVP(dst);
		ar & BOOST_SERIALIZATION_NVP(flags);
	}
};

}
}

#endif /* _MEMEVENT_H */
