// -*- mode: c++ -*-
// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef SST_CORE_INTERFACES_SIMPLENETWORK_H
#define SST_CORE_INTERFACES_SIMPLENETWORK_H

#include "sst/core/params.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"
#include "sst/core/subcomponent.h"
#include "sst/core/warnmacros.h"

#include <string>
#include <unordered_map>

namespace SST {

class Component;
class Event;
class Link;

namespace Interfaces {

/**
 * Generic network interface
 */
class SimpleNetwork : public SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Interfaces::SimpleNetwork,int)

    /** All Addresses can be 64-bit */
    typedef int64_t nid_t;
#define PRI_NID PRIi64

    static const nid_t INIT_BROADCAST_ADDR;

    /**
     * Represents both network sends and receives
     */
    class Request : public SST::Core::Serialization::serializable, SST::Core::Serialization::serializable_type<Request>
    {

    public:
        nid_t  dest;           /*!< Node ID of destination */
        nid_t  src;            /*!< Node ID of source */
        int    vn;             /*!< Virtual network of packet */
        size_t size_in_bits;   /*!< Size of packet in bits */
        bool   head;           /*!< True if this is the head of a stream */
        bool   tail;           /*!< True if this is the tail of a steram */
        bool   allow_adaptive; /*!< Indicates whether adaptive routing is allowed or not. */

    private:
        Event* payload; /*!< Payload of the request */

    public:
        /**
           Sets the payload field for this request
           @param payload_in Event to set as payload.
         */
        inline void givePayload(Event* event) { payload = event; }

        /**
           Returns the payload for the request.  This will also set
           the payload to nullptr, so the call will only return valid
           data one time after each givePayload call.
           @return Event that was set as payload of the request.
        */
        inline Event* takePayload()
        {
            Event* ret = payload;
            payload    = nullptr;
            return ret;
        }

        /**
           Returns the payload for the request for inspection.  This
           call does not set the payload to nullptr, so deleting the
           request will also delete the payload.  If the request is
           going to be deleted, use takePayload instead.
           @return Event that was set as payload of the request.
        */
        inline Event* inspectPayload() { return payload; }

        /**
         * Trace types
         */
        typedef enum {
            NONE,  /*!< No tracing enabled */
            ROUTE, /*!< Trace route information only */
            FULL   /*!< Trace all movements of packets through network */
        } TraceType;

        /** Constructor */
        Request() :
            dest(0),
            src(0),
            size_in_bits(0),
            head(false),
            tail(false),
            allow_adaptive(true),
            payload(nullptr),
            trace(NONE),
            traceID(0)
        {}

        Request(nid_t dest, nid_t src, size_t size_in_bits, bool head, bool tail, Event* payload = nullptr) :
            dest(dest),
            src(src),
            size_in_bits(size_in_bits),
            head(head),
            tail(tail),
            allow_adaptive(true),
            payload(payload),
            trace(NONE),
            traceID(0)
        {}

        virtual ~Request()
        {
            if ( payload != nullptr ) delete payload;
        }

        inline Request* clone()
        {
            Request* req = new Request(*this);
            // Copy constructor only makes a shallow copy, need to
            // clone the event.
            if ( payload != nullptr ) req->payload = payload->clone();
            return req;
        }

        void      setTraceID(int id) { traceID = id; }
        void      setTraceType(TraceType type) { trace = type; }
        int       getTraceID() { return traceID; }
        TraceType getTraceType() { return trace; }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            ser& dest;
            ser& src;
            ser& vn;
            ser& size_in_bits;
            ser& head;
            ser& tail;
            ser& payload;
            ser& trace;
            ser& traceID;
            ser& allow_adaptive;
        }

    protected:
        TraceType trace;
        int       traceID;

    private:
        ImplementSerializable(SST::Interfaces::SimpleNetwork::Request)
    };
    /**
       Class used to inspect network requests going through the network.
     */
    class NetworkInspector : public SubComponent
    {

    public:
        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Interfaces::SimpleNetwork::NetworkInspector,std::string)

        NetworkInspector(ComponentId_t id) : SubComponent(id) {}

        virtual ~NetworkInspector() {}

        virtual void inspectNetworkData(Request* req) = 0;
    };

    /**
       Base handler for event delivery.
     */
    using HandlerBase = SSTHandlerBase<bool, int>;

    /**
       Used to create handlers to notify the endpoint when the
       SimpleNetwork sends or recieves a packet..  The callback
       function is expected to be in the form of:

         bool func(int vn)

       In which case, the class is created with:

         new SimpleNetwork::Handler<classname>(this, &classname::function_name)

       Or, to add static data, the callback function is:

         bool func(int vn, dataT data)

       and the class is created with:

         new SimpleNetwork::Handler<classname, dataT>(this, &classname::function_name, data)

       In both cases, the boolean that's returned indicates whether
       the handler should be kept in the list or not.  On return
       of true, the handler will be kept.  On return of false, the
       handler will be removed from the clock list.
    */
    template <typename classT, typename dataT = void>
    using Handler = SSTHandler<bool, int, classT, dataT>;


public:
    /** Constructor, designed to be used via 'loadUserSubComponent or loadAnonymousSubComponent'. */
    SimpleNetwork(SST::ComponentId_t id) : SubComponent(id) {}

    /**
     * Sends a network request during untimed phases (init() and
     * complete()).
     * @see SST::Link::sendUntimedData()
     */
    virtual void sendUntimedData(Request* req) = 0;

    /**
     * Receive any data during untimed phases (init() and complete()).
     * @see SST::Link::recvUntimedData()
     */
    virtual Request* recvUntimedData() = 0;

    // /**
    //  * Returns a handle to the underlying SST::Link
    //  */
    // virtual Link* getLink(void) const = 0;

    /**
     * Send a Request to the network.
     */
    virtual bool send(Request* req, int vn) = 0;

    /**
     * Receive a Request from the network.
     *
     * Use this method for polling-based applications.
     * Register a handler for push-based notification of responses.
     *
     * @param vn Virtual network to receive on
     * @return nullptr if nothing is available.
     * @return Pointer to a Request response (that should be deleted)
     */
    virtual Request* recv(int vn) = 0;

    virtual void setup() override {}
    virtual void init(unsigned int UNUSED(phase)) override {}
    virtual void complete(unsigned int UNUSED(phase)) override {}
    virtual void finish() override {}

    /**
     * Checks if there is sufficient space to send on the specified
     * virtual network
     * @param vn Virtual network to check
     * @param num_bits Minimum size in bits required to have space
     * to send
     * @return true if there is space in the output, false otherwise
     */
    virtual bool spaceToSend(int vn, int num_bits) = 0;

    /**
     * Checks if there is a waiting network request request pending in
     * the specified virtual network.
     * @param vn Virtual network to check
     * @return true if a network request is pending in the specified
     * virtual network, false otherwise
     */
    virtual bool requestToReceive(int vn) = 0;

    /**
     * Registers a functor which will fire when a new request is
     * received from the network.  Note, the actual request that
     * was received is not passed into the functor, it is only a
     * notification that something is available.
     * @param functor Functor to call when request is received
     */
    virtual void setNotifyOnReceive(HandlerBase* functor) = 0;
    /**
     * Registers a functor which will fire when a request is
     * sent to the network.  Note, this only tells you when data
     * is sent, it does not guarantee any specified amount of
     * available space.
     * @param functor Functor to call when request is sent
     */
    virtual void setNotifyOnSend(HandlerBase* functor)    = 0;

    /**
     * Check to see if network is initialized.  If network is not
     * initialized, then no other functions other than init() can
     * can be called on the interface.
     * @return true if network is initialized, false otherwise
     */
    virtual bool isNetworkInitialized() const = 0;

    /**
     * Returns the endpoint ID.  Cannot be called until after the
     * network is initialized.
     * @return Endpoint ID
     */
    virtual nid_t getEndpointID() const = 0;

    /**
     * Returns the final BW of the link managed by the simpleNetwork
     * instance.  Cannot be called until after the network is
     * initialized.
     * @return Link bandwidth of associated link
     */
    virtual const UnitAlgebra& getLinkBW() const = 0;
};

} // namespace Interfaces
} // namespace SST

#endif // SST_CORE_INTERFACES_SIMPLENETWORK_H
