// -*- mode: c++ -*-
// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef SST_CORE_INTERFACES_STANDARDMEM_H
#define SST_CORE_INTERFACES_STANDARDMEM_H

#include "sst/core/link.h"
#include "sst/core/params.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"
#include "sst/core/subcomponent.h"
#include "sst/core/warnmacros.h"

#include <atomic>
#include <map>
#include <string>
#include <utility>

namespace SST {

class Component;
class Event;

namespace Interfaces {
/**
 * Generic interface to Memory models
 *
 * Implementation notes
 * Instructions can be sent into a memory system using derivatives of Request().
 * This interface can be used by both compute hosts (e.g., CPUs) and MMIO devices (e.g., accelerator).
 * Not all interfaces/memory systems support all Request types. The interface should return
 * an error if it encounters an unhandled type.
 *
 * Request class:
 *  - Uses a separate class for each request type
 *  - Additional classes can be defined outside sst-core to add custom types
 *  - Requests and responses share the same ID
 *  - req.makeResponse() should be used to generate a correctly populated response event.
 *      - Any additional fields that need to be set manually are noted in function comments
 *  - req.needsResponse() should be used to determine whether a response should be sent.
 *
 * Built-in commands
 * Basic:
 *  - Reads, writes
 *  - Noncacheable reads, writes
 * Flushes:
 *  - By address: flush and flush-invalidate
 * Atomic updates:
 *  - Read-lock, Write-unlock
 *  - Load-link, Store-conditional
 * Data movement:
 *  - Data move (copy data from one memory location to another, e.g., for scratchpad)
 * Notifications:
 *  - Cache invalidation
 * Custom:
 *  - CustomRequest, this is intended to be extended by users
 *  - CustomResponse, this is intended to be extended by users
 */
class StandardMem : public SubComponent
{
public:
    class Request; // Needed for HandlerBase definition
    /**
       Base handler for request handling.
     */
    using HandlerBase = SSTHandlerBase<void, Request*>;

    /**
       Used to create handlers for request handling.  The callback
       function is expected to be in the form of:

         void func(Request* event)

       In which case, the class is created with:

         new StdMem::Handler<classname>(this, &classname::function_name)

       Or, to add static data, the callback function is:

         void func(Request* event, dataT data)

       and the class is created with:

         new stdMem::Handler<classname, dataT>(this, &classname::function_name, data)
     */
    template <typename classT, typename dataT = void>
    using Handler = SSTHandler<void, Request*, classT, dataT>;

    class RequestConverter; // Convert request to SST::Event* according to type
    class RequestHandler;   // Handle a request according to type

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Interfaces::StandardMem,TimeConverter*,HandlerBase*)

    /** All Addresses can be 64-bit */
    typedef uint64_t Addr;

    /**
     * Base class for StandardMem commands
     */
    class Request
    {
    public:
        typedef uint64_t id_t;
        typedef uint32_t flags_t;

        /** Flags that modify requests.
         * Each bit in a 32-bit field (flags_t) defines a seperate flag.
         * Values less than F_RESERVED are reserved for futgure expansion.
         * Users may define custom flags above F_RESERVED
         */
        enum class Flag {
            F_NONCACHEABLE = 1 << 1, /* Bypass caches for this event */
            F_FAIL         = 1 << 2, /* For events that can fail, this indicates failure. */
            F_TRACE = 1 << 3, /* This flag requests that debug/trace output be generated for this event if possible. */
            F_RESERVED = 1 << 16, /* Flags <= F_RESERVED are reserved for future expansion */
        };

        Request(flags_t fl = 0)
        {
            id    = main_id++;
            flags = fl;
        } /* Requests get a new ID */
        Request(id_t rid, flags_t flags = 0) :
            id(rid),
            flags(flags) {} /* Responses share an ID with the matching request */

        virtual ~Request() {}

        id_t getID() { return id; }

        virtual Request*
        makeResponse() = 0; // Helper for properly formatting responses; returns nullptr if no response type exists
        virtual bool needsResponse() = 0; // Indicates whether event requires a response

        /* Convert Request to an Event type - intended to be called by standardInterface */
        virtual SST::Event* convert(RequestConverter* converter) = 0;

        virtual void handle(RequestHandler* handler) = 0;

        /* Return string representation of event for debug/output/etc. */
        virtual std::string getString() = 0;

        /* Flag handling */
        void setNoncacheable() { flags |= static_cast<int>(Flag::F_NONCACHEABLE); }
        void unsetNoncacheable() { flags &= ~(static_cast<int>(Flag::F_NONCACHEABLE)); }
        bool getNoncacheable() { return flags & static_cast<int>(Flag::F_NONCACHEABLE); }

        void setSuccess() { unsetFail(); }
        void unsetSuccess() { setFail(); }
        bool getSuccess() { return (flags & static_cast<int>(Flag::F_FAIL)) == 0; }
        bool getFail() { return flags & static_cast<int>(Flag::F_FAIL); }
        void setFail() { flags |= static_cast<int>(Flag::F_FAIL); }
        void unsetFail() { flags &= ~(static_cast<int>(Flag::F_FAIL)); }

        void setTrace() { flags |= static_cast<int>(Flag::F_TRACE); }
        void unsetTrace() { flags &= (~static_cast<int>(Flag::F_TRACE)); }
        bool getTrace() { return flags & static_cast<int>(Flag::F_TRACE); }

        void setFlag(flags_t flag) { flags |= flag; }
        void setFlag(Flag flag) { flags |= static_cast<flags_t>(flag); }
        void unsetFlag(flags_t flag) { flags &= (~flag); }
        void unsetFlag(Flag flag) { flags &= ~(static_cast<flags_t>(flag)); }
        bool getFlag(flags_t flag) { return flags & flag; }
        bool getFlag(Flag flag) { return flags & static_cast<flags_t>(flag); }

        void    clearAllFlags() { flags = 0; }
        flags_t getAllFlags() { return flags; }

        std::string getFlagString()
        {
            /* Print flag name for known ones and F_XX where XX is the bit index for unknown ones */
            std::ostringstream str;
            bool               comma = false;
            if ( getNoncacheable() ) {
                str << "F_NONCACHEABLE";
                comma = true;
            }
            if ( getFail() ) {
                if ( comma ) { str << ","; }
                else {
                    comma = true;
                }
                str << "F_FAIL";
            }
            if ( getTrace() ) {
                if ( comma ) { str << ","; }
                else {
                    comma = true;
                }
                str << "F_TRACE";
            }
            for ( unsigned int i = 4; i < sizeof(flags_t); i++ ) {
                flags_t shift = 1 << i;
                if ( getFlag(shift) ) {
                    if ( comma ) { str << ","; }
                    else {
                        comma = true;
                    }
                    str << "F_" << i;
                }
            }
            return str.str();
        }

    protected:
        id_t    id;
        flags_t flags;

    private:
        static std::atomic<id_t> main_id;
    };

    /* Forward declarations */
    class Read;             /* Request to read data */
    class ReadResp;         /* Response to read requests */
    class Write;            /* Request to write data */
    class WriteResp;        /* Response to write requests */
    class FlushAddr;        /* Flush an address from cache */
    class FlushResp;        /* Response to flush request */
    class ReadLock;         /* Read and lock an address */
    class WriteUnlock;      /* Write and unlock an address */
    class LoadLink;         /* First part of LLSC, read and track access atomicity */
    class StoreConditional; /* Second part of LLSC, check access atomicity and write if atomic */
    class MoveData;         /* Copy data from one address to another (e.g., Get/Put) */
    class CustomReq;        /* Encapsulates a custom class that defines some request event type */
    class CustomResp;       /* Encapsulates a custom class that defines some response event type */
    class InvNotify;        /* Notification that data has been invalidated */

    /** Read request.
     *  Can be marked noncacheable to bypass caches.
     *  Response type is ReadResp
     */
    class Read : public Request
    {
    public:
        Read(Addr physAddr, uint64_t size, flags_t flags = 0, Addr virtAddr = 0, Addr instPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            iPtr(instPtr),
            tid(tid)
        {}
        virtual ~Read() {}

        /** Create read response.
         * User must manually set read data on response if simulation is using actual data values
         * @return ReadResp formatted as a response to this Read request
         */
        Request* makeResponse() override
        {
            std::vector<uint8_t> datavec(
                size, 0); /* Placeholder. If actual data values are used in simulation, the model should update this */
            ReadResp* resp = new ReadResp(this, datavec);
            return resp;
        }

        bool needsResponse() override { return true; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: Read, Flags: [" << getFlagString() << "], PhysAddr: 0x" << std::hex << pAddr
                << ", VirtAddr: 0x" << vAddr;
            str << ", Size: " << std::dec << size << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec
                << tid;
            return str.str();
        }

        /* Data members */
        Addr     pAddr; /* Physical address */
        Addr     vAddr; /* Virtual address */
        uint64_t size;  /* Number of bytes to read */
        Addr     iPtr;  /* Instruction pointer - optional metadata */
        uint32_t tid;   /* Thread ID */
    };

    /** Response to a Read */
    class ReadResp : public Request
    {
    public:
        ReadResp(
            id_t rid, Addr physAddr, uint64_t size, std::vector<uint8_t> respData, flags_t flags = 0, Addr virtAddr = 0,
            Addr instPtr = 0, uint32_t tid = 0) :
            Request(rid, flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            data(respData),
            iPtr(instPtr),
            tid(tid)
        {}

        ReadResp(Read* readEv, std::vector<uint8_t> respData) :
            Request(readEv->getID(), readEv->getAllFlags()),
            pAddr(readEv->pAddr),
            vAddr(readEv->vAddr),
            size(readEv->size),
            data(respData),
            iPtr(readEv->iPtr),
            tid(readEv->tid)
        {}

        virtual ~ReadResp() {}

        Request* makeResponse() override { return nullptr; } /* No response type */

        bool needsResponse() override { return false; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: ReadResp, Flags: [" << getFlagString() << "] PhysAddr: 0x" << std::hex
                << pAddr;
            str << ", VirtAddr: 0x" << vAddr << ", Size: " << std::dec << size << ", InstPtr: 0x" << std::hex << iPtr;
            str << ", ThreadID: " << std::dec << tid << ", Payload: 0x" << std::hex;
            str << std::setfill('0');
            for ( std::vector<uint8_t>::iterator it = data.begin(); it != data.end(); it++ ) {
                str << std::setw(2) << static_cast<unsigned>(*it);
            }
            return str.str();
        }

        /* Data members */
        Addr                 pAddr; /* Physical address */
        Addr                 vAddr; /* Virtual address */
        uint64_t             size;  /* Number of bytes to read */
        std::vector<uint8_t> data;  /* Read data */
        Addr                 iPtr;  /* Instruction pointer - optional metadata */
        uint32_t             tid;   /* Thread ID */
    };

    /** Request to write data.
     *  Can be marked noncacheable to bypass caches
     *  Response type is WriteResp
     */
    class Write : public Request
    {
    public:
        /* Constructor */
        Write(
            Addr physAddr, uint64_t size, std::vector<uint8_t> wData, bool posted = false, flags_t flags = 0,
            Addr virtAddr = 0, Addr instPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            data(wData),
            posted(posted),
            iPtr(instPtr),
            tid(tid)
        {}
        /* Destructor */
        virtual ~Write() {}

        virtual Request* makeResponse() override { return new WriteResp(this); }

        virtual bool needsResponse() override { return !posted; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: Write, Flags: [" << getFlagString() << "], PhysAddr: 0x" << std::hex
                << pAddr;
            str << ", VirtAddr: 0x" << vAddr << ", Size: " << std::dec << size << ", Posted: " << (posted ? "T" : "F");
            str << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid << ", Payload: 0x"
                << std::hex;
            str << std::setfill('0');
            for ( std::vector<uint8_t>::iterator it = data.begin(); it != data.end(); it++ ) {
                str << std::setw(2) << static_cast<unsigned>(*it);
            }
            return str.str();
        }

        /* Data members */
        Addr                 pAddr;  /* Physical address */
        Addr                 vAddr;  /* Virtual address */
        uint64_t             size;   /* Number of bytes to write */
        std::vector<uint8_t> data;   /* Written data */
        bool                 posted; /* Whether write is posted (requires no response) */
        Addr                 iPtr;   /* Instruction pointer - optional metadata */
        uint32_t             tid;    /* Thread ID */
    };

    /** Response to a Write */
    class WriteResp : public Request
    {
    public:
        /** Manually construct a write response */
        WriteResp(
            id_t id, Addr physAddr, uint64_t size, flags_t flags = 0, Addr virtAddr = 0, Addr instPtr = 0,
            uint32_t tid = 0) :
            Request(id, flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            iPtr(instPtr),
            tid(tid)
        {}
        /** Automatically construct a write response from a Write */
        WriteResp(Write* wr) :
            Request(wr->getID(), wr->getAllFlags()),
            pAddr(wr->pAddr),
            vAddr(wr->vAddr),
            size(wr->size),
            iPtr(wr->iPtr),
            tid(wr->tid)
        {}
        /** Destructor */
        virtual ~WriteResp() {}

        virtual Request* makeResponse() override { return nullptr; }

        virtual bool needsResponse() override { return false; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID:" << id << ", Type: WriteResp, Flags: [" << getFlagString() << "], PhysAddr: 0x" << std::hex
                << pAddr;
            str << ", VirtAddr: 0x" << vAddr << ", Size: " << std::dec << size << ", InstPtr: 0x" << std::hex << iPtr;
            str << ", ThreadID: " << std::dec << tid;
            return str.str();
        }

        /* Data members */
        Addr     pAddr; /* Physical address */
        Addr     vAddr; /* Virtual address */
        uint64_t size;  /* Number of bytes to read */
        Addr     iPtr;  /* Instruction pointer - optional metadata */
        uint32_t tid;   /* Thread ID */
    };

    /* Flush an address from cache
     * Response type is FlushResp
     * inv = false: Write back dirty data to memory, leave clean data in cache
     * inv = true: Write back dirty data to memory, invalidate data in cache
     */
    class FlushAddr : public Request
    {
    public:
        FlushAddr(
            Addr physAddr, uint64_t size, bool inv, uint32_t depth, flags_t flags = 0, Addr virtAddr = 0,
            Addr instPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            inv(inv),
            depth(depth),
            iPtr(instPtr),
            tid(tid)
        {}
        virtual ~FlushAddr() {}

        virtual Request* makeResponse() override { return new FlushResp(this); }

        virtual bool needsResponse() override { return true; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID:" << id << ", Type: FlushAddr, Flags: [" << getFlagString() << "], PhysAddr: 0x" << std::hex
                << pAddr;
            str << ", VirtAddr: 0x" << vAddr << ", Size: " << std::dec << size << ", Inv: " << (inv ? "T" : "F");
            str << ", Depth: " << depth << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid;
            return str.str();
        }

        Addr     pAddr; /* Physical address */
        Addr     vAddr; /* Virtual address */
        uint64_t size;  /* Number of bytes to invalidate */
        bool     inv;   /* Whether flush should also invalidate line */
        uint32_t depth; /* How many levels down the memory hierarchy this flush should propogate. E.g., 1 = L1 only, 2 =
                           L1 + L2, etc. */
        Addr     iPtr;  /* Instruction pointer */
        uint32_t tid;   /* Thread ID */
    };

    /** Response to a flush request.
     * Flushes can occasionally fail, check getSuccess() to determine success.
     */
    class FlushResp : public Request
    {
    public:
        FlushResp(
            id_t id, Addr physAddr, uint64_t size, flags_t flags = 0, Addr vAddr = 0, Addr instPtr = 0,
            uint32_t tid = 0) :
            Request(id, flags),
            pAddr(physAddr),
            vAddr(vAddr),
            size(size),
            iPtr(instPtr),
            tid(tid)
        {}
        FlushResp(FlushAddr* fl, flags_t newFlags = 0) :
            Request(fl->getID(), fl->getAllFlags() | newFlags),
            pAddr(fl->pAddr),
            vAddr(fl->vAddr),
            size(fl->size),
            iPtr(fl->iPtr),
            tid(fl->tid)
        {}
        virtual ~FlushResp() {}

        virtual Request* makeResponse() override { return nullptr; }

        virtual bool needsResponse() override { return false; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID:" << id << ", Type: FlushResp, Flags: [" << getFlagString() << "], PhysAddr: 0x" << std::hex
                << pAddr;
            str << ", VirtAddr: 0x" << vAddr << ", Size: " << std::dec << size;
            str << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid;
            return str.str();
        }

        Addr     pAddr; /* Physical address */
        Addr     vAddr; /* Virtual address */
        uint64_t size;  /* Number of bytes to invalidate */
        Addr     iPtr;  /* Instruction pointer */
        uint32_t tid;   /* Thread ID */
    };

    /**
     * Locked atomic update -> guaranteed success
     * A ReadLock **must** be followed by a WriteUnlock
     */

    /** ReadLock acquires and locks an address
     *  Returns a ReadResp with the current data value
     */
    class ReadLock : public Request
    {
    public:
        ReadLock(
            Addr physAddr, uint64_t size, flags_t flags = 0, Addr virtAddr = 0, Addr instPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            iPtr(instPtr),
            tid(tid)
        {}
        virtual ~ReadLock() {}

        Request* makeResponse() override
        {
            std::vector<uint8_t> datavec(size, 0); /* This is a placeholder. If actual data values are used in
                                                      simulation, the model should update this */
            return new ReadResp(id, pAddr, size, datavec, flags, vAddr, iPtr, tid);
        }

        bool needsResponse() override { return true; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: ReadLock, Flags: [" << getFlagString() << "] PhysAddr: 0x" << std::hex
                << pAddr << ", VirtAddr: 0x" << vAddr;
            str << ", Size: " << std::dec << size << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec
                << tid;
            return str.str();
        }

        /* Data members */
        Addr     pAddr; /* Physical address */
        Addr     vAddr; /* Virtual address */
        uint64_t size;  /* Number of bytes to read */
        Addr     iPtr;  /* Instruction pointer - optional metadata */
        uint32_t tid;   /* Thread ID */
    };

    /* WriteUnlock writes a locked address
     * WriteUnlock will fatally error if lock is not acquired first
     * Returns a WriteResp
     */
    class WriteUnlock : public Request
    {
    public:
        WriteUnlock(
            Addr physAddr, uint64_t size, std::vector<uint8_t> wData, bool posted = false, flags_t flags = 0,
            Addr virtAddr = 0, Addr instPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            data(wData),
            posted(posted),
            iPtr(instPtr),
            tid(tid)
        {}

        virtual ~WriteUnlock() {}

        virtual Request* makeResponse() override { return new WriteResp(id, pAddr, size, flags, vAddr = 0, iPtr, tid); }

        virtual bool needsResponse() override { return !posted; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: WriteUnlock, Flags: [" << getFlagString() << "], PhysAddr: 0x" << std::hex
                << pAddr;
            str << ", VirtAddr: 0x" << vAddr << ", Size: " << std::dec << size << ", Posted: " << (posted ? "T" : "F");
            str << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid << ", Payload: 0x"
                << std::hex;
            str << std::setfill('0');
            for ( std::vector<uint8_t>::iterator it = data.begin(); it != data.end(); it++ ) {
                str << std::setw(2) << static_cast<unsigned>(*it);
            }
            return str.str();
        }

        /* Data members */
        Addr                 pAddr;  /* Physical address */
        Addr                 vAddr;  /* Virtual address */
        uint64_t             size;   /* Number of bytes to write */
        std::vector<uint8_t> data;   /* Written data */
        bool                 posted; /* Whether write is posted (requires no response) */
        Addr                 iPtr;   /* Instruction pointer - optional metadata */
        uint32_t             tid;    /* Thread ID */
    };

    /**
     * Conditional atomic update. Can fail.
     * A LoadLink should be followed by a StoreConditional
     */

    /* LoadLink loads an address and tracks it for atomicity
     * Returns a ReadResp
     */
    class LoadLink : public Request
    {
    public:
        LoadLink(
            Addr physAddr, uint64_t size, flags_t flags = 0, Addr virtAddr = 0, Addr instPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            iPtr(instPtr),
            tid(tid)
        {}
        virtual ~LoadLink() {}

        Request* makeResponse() override
        {
            std::vector<uint8_t> datavec(size, 0); /* This is a placeholder. If actual data values are used in
                                                      simulation, the model should update this */
            return new ReadResp(id, pAddr, size, datavec, flags, vAddr, iPtr, tid);
        }

        bool needsResponse() override { return true; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: LoadLink, Flags: [" << getFlagString() << "] PhysAddr: 0x" << std::hex
                << pAddr << ", VirtAddr: 0x" << vAddr;
            str << ", Size: " << std::dec << size << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec
                << tid;
            return str.str();
        }

        /* Data members */
        Addr     pAddr; /* Physical address */
        Addr     vAddr; /* Virtual address */
        uint64_t size;  /* Number of bytes to read */
        Addr     iPtr;  /* Instruction pointer - optional metadata */
        uint32_t tid;   /* Thread ID */
    };

    /* StoreConditional checks if a write to a prior LoadLink address will be atomic,
     * if so, writes the address and returns a WriteResp with getSuccess() == true
     * if not, does not write the address and returns a WriteResp with getSuccess() == false
     */
    class StoreConditional : public Request
    {
    public:
        StoreConditional(
            Addr physAddr, uint64_t size, std::vector<uint8_t> wData, flags_t flags = 0, Addr virtAddr = 0,
            Addr instPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pAddr(physAddr),
            vAddr(virtAddr),
            size(size),
            data(wData),
            iPtr(instPtr),
            tid(tid)
        {}

        virtual ~StoreConditional() {}

        /* Model must also call setFail() on response if LLSC failed */
        virtual Request* makeResponse() override { return new WriteResp(id, pAddr, size, flags, vAddr, iPtr, tid); }

        virtual bool needsResponse() override { return true; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: StoreConditional, Flags: [" << getFlagString() << "], PhysAddr: 0x"
                << std::hex << pAddr;
            str << ", VirtAddr: 0x" << vAddr << ", Size: " << std::dec << size;
            str << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid << ", Payload: 0x"
                << std::hex;
            str << std::setfill('0');
            for ( std::vector<uint8_t>::iterator it = data.begin(); it != data.end(); it++ ) {
                str << std::setw(2) << static_cast<unsigned>(*it);
            }
            return str.str();
        }

        /* Data members */
        Addr                 pAddr; /* Physical address */
        Addr                 vAddr; /* Virtual address */
        uint64_t             size;  /* Number of bytes to write */
        std::vector<uint8_t> data;  /* Written data */
        Addr                 iPtr;  /* Instruction pointer - optional metadata */
        uint32_t             tid;   /* Thread ID */
    };

    /* Explicit data movement */
    /** Move: move data from one address to another
     *  Returns a WriteResp
     */
    class MoveData : public Request
    {
    public:
        MoveData(
            Addr pSrc, Addr pDst, uint64_t size, bool posted = false, flags_t flags = 0, Addr vSrc = 0, Addr vDst = 0,
            Addr iPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pSrc(pSrc),
            vSrc(vSrc),
            pDst(pDst),
            vDst(vDst),
            size(size),
            posted(posted),
            iPtr(iPtr),
            tid(tid)
        {}
        virtual ~MoveData() {}

        virtual Request* makeResponse() override { return new WriteResp(id, pDst, size, flags, vDst, iPtr, tid); }

        virtual bool needsResponse() override { return !posted; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: MoveData, Flags: [" << getFlagString() << "], SrcPhysAddr: 0x" << std::hex
                << pSrc;
            str << ", SrcVirtAddr: 0x" << vSrc << ", DstPhysAddr: 0x" << pDst << ", DstVirtAddr: 0x" << vDst;
            str << ", Size: " << std::dec << size << ", Posted: " << (posted ? "T" : "F");
            str << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid;
            return str.str();
        }

        /* Data members */
        Addr     pSrc;   /* Physical address of source */
        Addr     vSrc;   /* Virtual address of source */
        Addr     pDst;   /* Physical address of destination */
        Addr     vDst;   /* Virtual address of destination */
        uint64_t size;   /* Number of bytes to move */
        bool     posted; /* True if a response is needed */
        Addr     iPtr;   /* Instruction pointer */
        uint32_t tid;    /* Thread ID */
    };

    /** Notifies endpoint that an address has been invalidated from the L1.
     */
    class InvNotify : public Request
    {
    public:
        InvNotify(Addr pAddr, uint64_t size, flags_t flags = 0, Addr vAddr = 0, Addr iPtr = 0, uint32_t tid = 0) :
            Request(flags),
            pAddr(pAddr),
            vAddr(vAddr),
            size(size),
            iPtr(iPtr),
            tid(tid)
        {}
        virtual ~InvNotify() {}

        virtual Request* makeResponse() override { return nullptr; }

        virtual bool needsResponse() override { return false; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: InvNotify, Flags: [" << getFlagString() << "], PhysAddr: 0x" << std::hex
                << pAddr;
            str << ", VirtAddr: 0x" << vAddr << ", Size: " << std::dec << size;
            str << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid;
            return str.str();
        }

        Addr     pAddr; /* Physical address */
        Addr     vAddr; /* Virtual address */
        uint64_t size;  /* Number of bytes invalidated */
        Addr     iPtr;  /* Instruction pointer */
        uint32_t tid;   /* Thread ID */
    };

    /* This class can be inherited to create custom events that can be handled in a limited fashion by existing
     * interfaces Child class must be serializable
     */
    class CustomData : public SST::Core::Serialization::serializable
    {
    public:
        CustomData() {}
        virtual ~CustomData() {}
        virtual Addr     getRoutingAddress() = 0; /* Return address to use for routing this event to its destination */
        virtual uint64_t getSize()           = 0; /* Return size to use when accounting for bandwidth used is needed */
        virtual CustomData* makeResponse()   = 0; /* Return a CustomData* object formatted as a response */
        virtual bool        needsResponse()  = 0; /* Return whether a response is needed */
        virtual std::string getString()      = 0; /* String representation for debug/output/etc. */

        /* This needs to be serializable so that we can use it in events in parallel simulations */
        virtual void serialize_order(SST::Core::Serialization::serializer& UNUSED(ser)) override = 0;
        // ImplementSerializable(SST::Interfaces::StandardMem::CustomData);
        ImplementVirtualSerializable(CustomData);
    };

    class CustomReq : public Request
    {
    public:
        CustomReq(CustomData* data, flags_t flags = 0, Addr iPtr = 0, uint32_t tid = 0) :
            Request(flags),
            data(data),
            iPtr(iPtr),
            tid(tid)
        {}
        virtual ~CustomReq() {}

        virtual Request* makeResponse() override { return new CustomResp(this); }

        virtual bool needsResponse() override { return data->needsResponse(); }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: CustomReq, Flags: [" << getFlagString() << "], " << data->getString();
            str << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid;
            return str.str();
        }

        CustomData* data; /* Custom class that holds data for this event */
        Addr        iPtr; /* Instruction pointer */
        uint32_t    tid;  /* Thread ID */
    };

    class CustomResp : public Request
    {
    public:
        CustomResp(id_t id, CustomData* data, flags_t flags = 0, Addr iPtr = 0, uint32_t tid = 0) :
            Request(id, flags),
            data(data),
            iPtr(iPtr),
            tid(tid)
        {}
        CustomResp(CustomReq* req) :
            Request(req->getID(), req->getAllFlags()),
            data(req->data->makeResponse()),
            iPtr(req->iPtr),
            tid(req->tid)
        {}
        virtual ~CustomResp() {}

        virtual Request* makeResponse() override { return nullptr; }

        virtual bool needsResponse() override { return false; }

        SST::Event* convert(RequestConverter* converter) override { return converter->convert(this); }

        void handle(RequestHandler* handler) override { return handler->handle(this); }

        std::string getString() override
        {
            std::ostringstream str;
            str << "ID: " << id << ", Type: CustomResp, Flags: [" << getFlagString() << "], " << data->getString();
            str << ", InstPtr: 0x" << std::hex << iPtr << ", ThreadID: " << std::dec << tid;
            return str.str();
        }

        CustomData* data; /* Custom class that holds data for this event */
        Addr        iPtr; /* Instruction pointer */
        uint32_t    tid;  /* Thread ID */
    };

    /* Class for implementation-specific converter functions */
    class RequestConverter
    {
    public:
        RequestConverter() {}
        virtual ~RequestConverter() {}

        /* Built in command converters */
        virtual SST::Event* convert(Read* request)             = 0;
        virtual SST::Event* convert(ReadResp* request)         = 0;
        virtual SST::Event* convert(Write* request)            = 0;
        virtual SST::Event* convert(WriteResp* request)        = 0;
        virtual SST::Event* convert(FlushAddr* request)        = 0;
        virtual SST::Event* convert(FlushResp* request)        = 0;
        virtual SST::Event* convert(ReadLock* request)         = 0;
        virtual SST::Event* convert(WriteUnlock* request)      = 0;
        virtual SST::Event* convert(LoadLink* request)         = 0;
        virtual SST::Event* convert(StoreConditional* request) = 0;
        virtual SST::Event* convert(MoveData* request)         = 0;
        virtual SST::Event* convert(CustomReq* request)        = 0;
        virtual SST::Event* convert(CustomResp* request)       = 0;
        virtual SST::Event* convert(InvNotify* request)        = 0;
    };

    /* Class for implementation-specific handler functions */
    class RequestHandler
    {
    public:
        RequestHandler(SST::Output* o) : out(o) {}
        virtual ~RequestHandler() {}

        /* Built in command handlers */
        virtual void handle(Read* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for Read requests is not implemented\n");
        }
        virtual void handle(ReadResp* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for ReadResp requests is not implemented\n");
        }
        virtual void handle(Write* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for Write requests is not implemented\n");
        }
        virtual void handle(WriteResp* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for WriteResp requests is not implemented\n");
        }
        virtual void handle(FlushAddr* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for FlushAddr requests is not implemented\n");
        }
        virtual void handle(FlushResp* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for FlushResp requests is not implemented\n");
        }
        virtual void handle(ReadLock* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for ReadLock requests is not implemented\n");
        }
        virtual void handle(WriteUnlock* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for WriteUnlock requests is not implemented\n");
        }
        virtual void handle(LoadLink* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for LoadLink requests is not implemented\n");
        }
        virtual void handle(StoreConditional* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for StoreConditional requests is not implemented\n");
        }
        virtual void handle(MoveData* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for MoveData requests is not implemented\n");
        }
        virtual void handle(CustomReq* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for CustomReq requests is not implemented\n");
        }
        virtual void handle(CustomResp* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for CustomResp requests is not implemented\n");
        }
        virtual void handle(InvNotify* UNUSED(request))
        {
            out->fatal(CALL_INFO, -1, "Error: RequestHandler for InvNotify requests is not implemented\n");
        }

        SST::Output* out;
    };

    /** Constructor, designed to be used via 'loadUserSubComponent' and 'loadAnonymousSubComponent'.
     *
     * @param id Component ID assigned to this subcomponent
     * @param params Parameters passed to this subcomponent
     * @param time TimeConverter indicating the time base (e.g., clock period) associated with this endpoint
     * @param handler Callback function to use for event receives
     */
    StandardMem(
        SST::ComponentId_t id, Params& UNUSED(params), TimeConverter*& UNUSED(time), HandlerBase*& UNUSED(handler)) :
        SubComponent(id)
    {}

    /**
     * Sends a memory-based request during the init()/complete() phases
     * @param req Request to send
     */
    virtual void sendUntimedData(Request* req) = 0;

    /**
     * Receive any data during the init()/complete() phases.
     * @see SST::Link::recvInitData()
     * Handler is not used during init()/complete(), parent must
     * poll this interface to get received events.
     *
     * @return Event if one was received, otherwise nullptr
     */
    virtual Request* recvUntimedData() = 0;

    /**
     * Send a Request through the interface
     *
     * @param req Request to send
     */
    virtual void send(Request* req) = 0;

    /**
     * Receive a Request response from the side of the link.
     *
     * Use this method for polling-based applications.
     * Register a handler for push-based notification of responses.
     *
     * @return nullptr if nothing is available.
     * @return Pointer to a Request response
     *          Upon receipt, the receiver takes responsibility for deleting the event
     */
    virtual Request* poll(void) = 0;

    /**
     * Get cache/memory line size (in bytes) from the memory system
     *
     * The memory system should provide this and it should be
     * valid after the init() phase is complete, so processors
     * can safely call this function during setup().
     *
     * @return 0 if the interface does not provide this capability or not relevant
     * @return line size of the memory system
     */
    virtual Addr getLineSize() = 0;

    /**
     * Sets the physical memory address(es), if any, that are mapped
     * to this endpoint. Not required for endpoints that are not mapped
     * into the memory address space.
     *
     * Components loading this subcomponent as an MMIO device must call this
     * function prior to SST's init() phase.
     *
     * @param start Base address of the region mapped to this endpoint
     * @param size Size, in bytes, of the region mapped to this endpoint
     */
    virtual void setMemoryMappedAddressRegion(Addr start, Addr size) = 0;
};

} // namespace Interfaces
} // namespace SST

#endif // SST_CORE_INTERFACES_STANDARDMEM_H
