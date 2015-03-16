// -*- mode: c++ -*-
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
//

#ifndef CORE_INTERFACES_SIMPLENETWORK_H_
#define CORE_INTERFACES_SIMPLENETWORK_H_

#include <string>

#include <sst/core/sst_types.h>
#include <sst/core/subcomponent.h>
#include <sst/core/params.h>

namespace SST {

class Component;
class Event;
class Link;
    
namespace Interfaces {
    
class NetworkInspector : public SubComponent {
public:
    NetworkInspector(Component* parent) :
        SubComponent(parent)
    {}

    virtual ~NetworkInspector() {}

    virtual void inspectNetworkData(Event* ev) = 0;
};

    
/**
 * Generic network interface
 */
class SimpleNetwork : public SubComponent {

public:
    /** All Addresses can be 64-bit */
    typedef uint64_t nid_t;

    static const nid_t INIT_BROADCAST_ADDR = 0xffffffffffffffffl;
    
    /**
     * Represents both network sends and receives
     */
    class Request {
    public:
        nid_t  dest;          /*!< Node ID of destination */
        nid_t  src;           /*!< Node ID of source */
        int    vn;            /*!< Virtual network of packet */
        size_t size_in_bits;  /*!< Size of packet in bits */
        bool   head;          /*!< True if this is the head of a stream */
        bool   tail;          /*!< True if this is the tail of a steram */
        Event* payload;       /*!< Payload of the request */
        
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

        inline Request* clone() {
            Request* req = new Request(*this);
            // Copy constructor only makes a shallow copy, need to
            // clone the event.
            if ( payload != NULL ) req->payload = payload->clone();
            return req;
        }
        
        inline void setTraceID(int id) {traceID = id;}
        inline void setTraceType(TraceType type) {trace = type;}
        inline int getTraceID() {return traceID;}
        inline TraceType getTraceType() {return trace;}
        
    protected:
        TraceType trace;
        int traceID;

    private:

        friend class boost::serialization::access;
        template<class Archive>
        void
        serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_NVP(dest);
            ar & BOOST_SERIALIZATION_NVP(src);
            ar & BOOST_SERIALIZATION_NVP(vn);
            ar & BOOST_SERIALIZATION_NVP(size_in_bits);
            ar & BOOST_SERIALIZATION_NVP(head);
            ar & BOOST_SERIALIZATION_NVP(tail);
            ar & BOOST_SERIALIZATION_NVP(payload);
        }
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

    virtual void setup() {}
    virtual void init(unsigned int phase) {}
    virtual void finish() {}

    /**
     * Checks if there is sufficient space to send on the specified
     * virtual netork
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
     * is sent, it does not guarentee any specified amount of
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
    virtual int getEndpointID() const = 0;

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
