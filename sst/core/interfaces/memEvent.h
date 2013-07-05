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
    /* Directory Controller */ \
    X(Fetch) \
    X(FetchInvalidate) \
    X(Evicted) \
	X(NULLCMD)

/** \enum Valid commands for the MemEvent */
typedef enum {
#define X(x) x,
	X_TYPES
#undef X
} Command;

/** Array of the stringify'd version of the MemEvent Commands.  Useful for printing. */
static const char* CommandString[] __attribute__((unused)) = {
#define X(x) #x ,
	X_TYPES
#undef X
};

#undef X_TYPES


static const std::string BROADCAST_TARGET = "BROADCAST";

/**
 * Interface Event used to represent Memory-based communication.
 *
 * This class primarily consists of a Command to perform at a particular address,
 * potentially including data.
 *
 * The command list includes the needed commands to execute cache coherency protocols
 * as well as standard reads and writes to memory.
 */
class MemEvent : public SST::Event {
public:
    /** Used to specify that this data should be written back to the backing store */
	static const uint32_t F_WRITEBACK = (1<<0);
    /** Used in a Read-Lock, Write-Unlock atomicity scheme */
	static const uint32_t F_LOCKED    = (1<<1);
    /** Used to delay snoops when a block is locked */
    static const uint32_t F_DELAYED   = (1<<2);
    /** Used to signify the desire to load a cache block directly in EXCLUSIVE mode */
    static const uint32_t F_EXCLUSIVE = (1<<3);
    /** Used to specify that this memory event should not be cached */
    static const uint32_t F_UNCACHED  = (1<<4);

    /** Data Payload type */
	typedef std::vector<uint8_t> dataVec;
    /** Each MemEvent has a unique (auto-generated) ID of this type */
	typedef std::pair<uint64_t, int> id_type;

    /** Creates a new MemEvent */
	MemEvent(const Component *_src, Addr _addr, Command _cmd) :
		SST::Event(), addr(_addr), cmd(_cmd), src(_src->getName())
	{
		event_id = std::make_pair(main_id++, _src->getId());
		response_to_id = std::make_pair(0, -1);
		dst = BROADCAST_TARGET;
		size = 0;
		flags = 0;
	}

    /** Copy Construtor. */
	MemEvent(const MemEvent &me) :
        SST::Event(), event_id(me.event_id), response_to_id(me.response_to_id),
        addr(me.addr), size(me.size), cmd(me.cmd), payload(me.payload),
        src(me.src), dst(me.dst), flags(me.flags)
	{
		setDeliveryLink(me.getLinkId(), NULL);
	}

    /** Copy Construtor. */
	MemEvent(const MemEvent *me) :
        SST::Event(), event_id(me->event_id), response_to_id(me->response_to_id),
        addr(me->addr), size(me->size), cmd(me->cmd), payload(me->payload),
        src(me->src), dst(me->dst), flags(me->flags)
	{
		setDeliveryLink(me->getLinkId(), NULL);
	}


    /**
     * Creates a new MemEvent instance, pre-configured to act as a response
     * to this MemEvent.
     * @param[in] source    Source Component where the response is being generated.
     * @return              A pointer to a new MemEvent
     */
	MemEvent* makeResponse(const Component *source)
	{
		MemEvent *me = new MemEvent(source, addr, commandResponse(cmd));
		me->setSize(size);
		me->response_to_id = event_id;
		me->dst = src;
        if ( queryFlag(F_UNCACHED) )
            me->setFlag(F_UNCACHED);
		return me;
	}

    /** @return  Unique ID of this MemEvent */
	id_type getID(void) const { return event_id; }
    /** @return  Unique ID of the MemEvent that this is a response to */
	id_type getResponseToID(void) const { return response_to_id; }
    /** @return  Command of this MemEvent */
	Command getCmd(void) const { return cmd; }
    /** Sets the Command of this MemEvent */
	void setCmd(Command newcmd) { cmd = newcmd; }
    /** @return  the target Address of this MemEvent */
	Addr getAddr(void) const { return addr; }
    /** Sets the target Address of this MemEvent */
	void setAddr(Addr newAddr) { addr = newAddr; }

    /** @return  the size in bytes that this MemEvent represents */
	uint32_t getSize(void) const { return size; }
    /** Sets the size in bytes that this MemEvent represents */
	void setSize(uint32_t sz) {
		size = sz;
	}

    /** @return  the data payload. */
	dataVec& getPayload(void)
    {
        /* Lazily allocate space for payload */
        if ( payload.size() < size )
            payload.resize(size);
        return payload;
    }
    /** Sets the data payload and payload size.
     * @param[in] size  How many bytes to copy from data
     * @param[in] data  Data array to set as payload
     */
	void setPayload(uint32_t size, uint8_t *data)
    {
		setSize(size);
        payload.resize(size);
		for ( uint32_t i = 0 ; i < size ; i++ ) {
			payload[i] = data[i];
		}
	}
    /** Sets the data payload and payload size.
     * @param[in] data  Vector from which to copy data
     */
	void setPayload(std::vector<uint8_t> &data) {
		setSize(data.size());
		payload = data;
	}

    /** @return the source string - who sent this MemEvent */
	const std::string& getSrc(void) const { return src; }
    /** Sets the source string - who sent this MemEvent */
	void setSrc(const std::string &s) { src = s; }
    /** @return the destination string - who receives this MemEvent */
	const std::string& getDst(void) const { return dst; }
    /** Sets the destination string - who received this MemEvent */
	void setDst(const std::string &d) { dst = d; }

    /** @return TRUE if this packet is targeted as a BROADCAST */
	bool isBroadcast(void) const { return (dst == BROADCAST_TARGET); }

    /** @returns the state of all flags for this MemEvent */
	uint32_t getFlags(void) const { return flags; }
    /** Sets the specified flag.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent */
	void setFlag(uint32_t flag) { flags = flags | flag; }
    /** Clears the speficied flag.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent */
	void clearFlag(uint32_t flag) { flags = flags & (~flag); }
    /** Clears all flags */
	void clearFlags(void) { flags = 0; }
    /** Check to see if a flag is set.
     * @param[in] flag  Should be one of the flags beginning with F_,
     *                  defined in MemEvent
     * @returns TRUE if the flag is set, FALSE otherwise
     */
	bool queryFlag(uint32_t flag) const { return flags & flag; };
    /** Sets the entire flag state */
	void setFlags(uint32_t _flags) { flags = _flags; }

    /** @returns the (optional) ID associated with the flag F_LOCKED */
	uint64_t getLockID(void) const { return lockid; }
    /** sets the (optional) ID associated with the flag F_LOCKED */
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
