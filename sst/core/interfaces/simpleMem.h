// -*- mode: c++ -*-
// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef CORE_INTERFACES_SIMPLEMEM_H_
#define CORE_INTERFACES_SIMPLEMEM_H_

#include <string>
#include <utility>
#include <map>

#include <sst/core/sst_types.h>
#include <sst/core/module.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

namespace SST {

class Component;
class Event;

namespace Interfaces {

class SimpleMem : public Module {

public:
    typedef uint64_t Addr;
    class Request {
    public:
        typedef uint64_t id_t;
        typedef uint32_t flags_t;
        typedef enum {
            Read,
            Write,
            ReadResp,
            WriteResp
        } Command;

        typedef enum {
            F_UNCACHED  = 1<<1,
            F_EXCLUSIVE = 1<<2,
            F_LOCKED    = 1<<3
        } Flags;

        typedef std::vector<uint8_t> dataVec;

        Command cmd;
        Addr addr;
        size_t size;
        dataVec data;
        flags_t flags;
        id_t id;

        Request(Command cmd, Addr addr, size_t size, dataVec &data, flags_t flags = 0) :
            cmd(cmd), addr(addr), size(size), data(data), flags(flags)
        {
            // TODO:  If we support threading in the future, this should be made atomic
            id = main_id++;
        }

        Request(Command cmd, Addr addr, size_t size, flags_t flags = 0) :
            cmd(cmd), addr(addr), size(size), flags(flags)
        {
            // TODO:  If we support threading in the future, this should be made atomic
            id = main_id++;
        }

        void setPayload(const std::vector<uint8_t> & data_in )
        {
            data = data_in;
        }

        void setPayload(uint8_t *data_in, size_t len)
        {
            data.resize(len);
            for ( size_t i = 0 ; i < len ; i++ ) {
                data[i] = data_in[i];
            }
        }

    private:
        static id_t main_id;
    };

    /** Functor classes for Clock handling */
    class HandlerBase {
    public:
        /** Function called when Handler is invoked */
        virtual void operator()(Request*) = 0;
        virtual ~HandlerBase() {}
    };


    /** Event Handler class with user-data argument
     * @tparam classT Type of the Object
     * @tparam argT Type of the argument
     */
    template <typename classT, typename argT = void>
    class Handler : public HandlerBase {
    private:
        typedef void (classT::*PtrMember)(Request*, argT);
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

        void operator()(Request* req) {
            return (object->*member)(req,data);
        }
    };

    /** Event Handler class without user-data
     * @tparam classT Type of the Object
     */
    template <typename classT>
    class Handler<classT, void> : public HandlerBase {
    private:
        typedef void (classT::*PtrMember)(Request*);
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

        void operator()(Request* req) {
            return (object->*member)(req);
        }
    };


    SimpleMem(SST::Component *comp, Params &params) { }
    virtual void initialize(const std::string &linkName, HandlerBase *handler = NULL) = 0;

    virtual void sendInitData(Request *req) = 0;
    virtual void sendInitData(SST::Event *ev) { getLink()->sendInitData(ev); }
    virtual SST::Event* recvInitData() { return getLink()->recvInitData(); }

    virtual SST::Link* getLink(void) const = 0;

    virtual void sendRequest(Request *req) = 0;
    virtual Request* recvResponse(void) = 0;


};

}
}

#endif
