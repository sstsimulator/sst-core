// -*- mode: c++ -*-
// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef CORE_INTERFACES_SIMPLENETWORK_H_
#define CORE_INTERFACES_SIMPLENETWORK_H_

#include <string>
#include <unordered_map>

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>
#include <sst/core/subcomponent.h>
#include <sst/core/params.h>

#include <sst/core/serialization/serializable.h>

namespace SST {

class Component;
class Event;
class Link;
    
namespace Interfaces {
    
    
/**
 * Generic network interface
 */
class SimpleNetwork : public SubComponent {

public:
    /** All Addresses can be 64-bit */
    typedef int64_t nid_t;

    static const nid_t INIT_BROADCAST_ADDR;
    
    /**
     * Represents both network sends and receives
     */
    class Request : public SST::Core::Serialization::serializable, SST::Core::Serialization::serializable_type<Request> {

    public:
        nid_t  dest;          /*!< Node ID of destination */
        nid_t  src;           /*!< Node ID of source */
        int    vn;            /*!< Virtual network of packet */
        size_t size_in_bits;  /*!< Size of packet in bits */
        bool   head;          /*!< True if this is the head of a stream */
        bool   tail;          /*!< True if this is the tail of a steram */
        
    private:
        Event* payload;       /*!< Payload of the request */
        
    public:

        /**
           Sets the payload field for this request
           @param payload_in Event to set as payload.
         */
        inline void givePayload(Event *event) {
            payload = event;
        }
        
        /**
           Returns the payload for the request.  This will also set
           the payload to NULL, so the call will only return valid
           data one time after each givePayload call.
           @return Event that was set as payload of the request.
        */
        inline Event* takePayload() {
            Event* ret = payload;
            payload = NULL;
            return ret;
        }

        /**
           Returns the payload for the request for inspection.  This
           call does not set the payload to NULL, so deleting the
           request will also delete the payload.  If the request is
           going to be deleted, use takePayload instead.
           @return Event that was set as payload of the request.
        */
        inline Event* inspectPayload() {
            return payload;
        }

        /**
         * Trace types
         */
        typedef enum {
            NONE,       /*!< No tracing enabled */
            ROUTE,      /*!< Trace route information only */
            FULL        /*!< Trace all movements of packets through network */
        } TraceType;


        /** Constructor */
        Request() :
            dest(0), src(0), size_in_bits(0), head(false), tail(false), payload(NULL),
            trace(NONE), traceID(0)
        {}

        Request(nid_t dest, nid_t src, size_t size_in_bits,
                bool head, bool tail, Event* payload = NULL) :
            dest(dest), src(src), size_in_bits(size_in_bits), head(head), tail(tail), payload(payload),
            trace(NONE), traceID(0)
        {
        }

        virtual ~Request()
        {
            if ( payload != NULL ) delete payload;
        }
        
        inline Request* clone() {
            Request* req = new Request(*this);
            // Copy constructor only makes a shallow copy, need to
            // clone the event.
            if ( payload != NULL ) req->payload = payload->clone();
            return req;
        }
        
        void setTraceID(int id) {traceID = id;}
        void setTraceType(TraceType type) {trace = type;}
        int getTraceID() {return traceID;}
        TraceType getTraceType() {return trace;}
        
        void serialize_order(SST::Core::Serialization::serializer &ser) override {
            ser & dest;
            ser & src;
            ser & vn;
            ser & size_in_bits;
            ser & head;
            ser & tail;
            ser & payload;
            ser & trace;
            ser & traceID;
        }
        
    protected:
        TraceType trace;
        int traceID;

    private:

        ImplementSerializable(SST::Interfaces::SimpleNetwork::Request)
    };
    /**
       Class used to inspect network requests going through the network.
     */
    class NetworkInspector : public SubComponent {

    public:
        NetworkInspector(Component* parent) :
            SubComponent(parent)
        {}

        virtual ~NetworkInspector() {}

        virtual void inspectNetworkData(Request* req) = 0;

        /**
         *  The ID uniquely identifies the component in which this
         *  subcomponent is instantiated.  It does not uniquely define
         *  this particular NetworkInspector, and all NetworkInspectors
         *  instantiated in the same component will get the same ID.  If
         *  registering statistics, the ID is intended to be used as the
         *  subfield of the statistic.
         */
        virtual void initialize(std::string id) = 0;
    };

    /** Functor classes for handling of callbacks */
    class HandlerBase {
    public:
        virtual bool operator()(int) = 0;
        virtual ~HandlerBase() {}
    };
    

    /** Event Handler class with user-data argument
     * @tparam classT Type of the Object
     * @tparam argT Type of the argument
     */
    template <typename classT, typename argT = void>
    class Handler : public HandlerBase {
    private:
        typedef bool (classT::*PtrMember)(int, argT);
        classT* object;
        const PtrMember member;
        argT data;
        
    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         * @param data - Additional argument to pass to handler
         */
        Handler( classT* const object, PtrMember member, argT data ) :
            object(object),
            member(member),
            data(data)
        {}
        
        bool operator()(int vn) {
            return (object->*member)(vn,data);
        }
    };
    
    /** Event Handler class without user-data
     * @tparam classT Type of the Object
     */
    template <typename classT>
    class Handler<classT, void> : public HandlerBase {
    private:
        typedef bool (classT::*PtrMember)(int);
        classT* object;
        const PtrMember member;
        
    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         */
        Handler( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}
        
        bool operator()(int vn) {
            return (object->*member)(vn);
        }
    };

public:
    
    /** Constructor, designed to be used via 'loadSubComponent'. */
    SimpleNetwork(SST::Component *comp) :
        SubComponent(comp)
    { }

    /** Second half of building the interface.
        Initialize network interface
        @param portName - Name of port to connect to
        @param link_bw - Bandwidth of the link
        @param vns - Number of virtual networks to be provided
        @param in_buf_size - Size of input buffers (from router)
        @param out_buf_size - Size of output buffers (to router)
     * @return true if the link was able to be configured.
     */
    virtual bool initialize(const std::string &portName, const UnitAlgebra& link_bw,
                            int vns, const UnitAlgebra& in_buf_size,
                            const UnitAlgebra& out_buf_size) = 0;

    /**
     * Sends a network request during the init() phase
     */
    virtual void sendInitData(Request *req) = 0;

    /**
     * Receive any data during the init() phase.
     * @see SST::Link::recvInitData()
     */
    virtual Request* recvInitData() = 0;

    /**
     * Sends a network request during untimed phases (init() and
     * complete()).
     * @see SST::Link::sendUntimedData()
     *
     * For now, simply call sendInitData.  Once that call is
     * deprecated and removed, this will become a pure virtual
     * function.  This means that when classes implement
     * SimpleNetwork, they will need to overload both sendUntimedData
     * and sendInitData with identical functionality (or having the
     * Init version call the Untimed version) until sendInitData is
     * removed in SST 9.0.
     */
    virtual void sendUntimedData(Request *req) {
        sendInitData(req);
    }
        
    /**
     * Receive any data during untimed phases (init() and complete()).
     * @see SST::Link::recvUntimedData()
     *
     * For now, simply call recvInitData.  Once that call is
     * deprecated and removed, this will become a pure virtual
     * function.  This means that when classes implement
     * SimpleNetwork, they will need to overload both recvUntimedData
     * and recvInitData with identical functionality (or having the
     * Init version call the Untimed version) until recvInitData is
     * removed in SST 9.0.
     */
    virtual Request* recvUntimedData() {
        return recvInitData();
    }

    // /**
    //  * Returns a handle to the underlying SST::Link
    //  */
    // virtual Link* getLink(void) const = 0;

    /**
     * Send a Request to the network.
     */
    virtual bool send(Request *req, int vn) = 0;

    /**
     * Receive a Request from the network.
     *
     * Use this method for polling-based applications.
     * Register a handler for push-based notification of responses.
     *
     * @param vn Virtual network to receive on
     * @return NULL if nothing is available.
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
    virtual bool requestToReceive( int vn ) = 0;

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
    virtual void setNotifyOnSend(HandlerBase* functor) = 0;

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

}
}

#endif
