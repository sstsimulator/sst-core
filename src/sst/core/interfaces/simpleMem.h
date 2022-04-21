// -*- mode: c++ -*-
// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef SST_CORE_INTERFACES_SIMPLEMEM_H
#define SST_CORE_INTERFACES_SIMPLEMEM_H

// DEPRECATION NOTICE
// This interface is deprecated and will be removed in SST 13.
// The StandardMem interface (stdMem.h) provides an expanded set of functionality.
// Please use StandardMem instead.

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
 * Simplified, generic interface to Memory models
 */
class __attribute__((deprecated("The SimpleMem interface is deprecated in favor of the StandardMem interface (sst/core/interfaces/stdMem.h). Please switch interfaces. This interface will be removed in SST 13."))) SimpleMem : public SubComponent
{
public:
    class Request; // Needed for HandlerBase definition
    /**
       Base handler for request handling.
     */
    using HandlerBase = SSTHandlerBase<void, Request*, false>;

    /**
       Used to create handlers for request handling.  The callback
       function is expected to be in the form of:

         void func(Request* event)

       In which case, the class is created with:

         new SimpleMem::Handler<classname>(this, &classname::function_name)

       Or, to add static data, the callback function is:

         void func(Request* event, dataT data)

       and the class is created with:

         new SimpleMem::Handler<classname, dataT>(this, &classname::function_name, data)
     */
    template <typename classT, typename dataT = void>
    using Handler = SSTHandler<void, Request*, false, classT, dataT>;
    
    DISABLE_WARN_DEPRECATED_DECLARATION
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Interfaces::SimpleMem,TimeConverter*,HandlerBase*)
    REENABLE_WARNING
    
    /** All Addresses can be 64-bit */
    typedef uint64_t Addr;

    /**
     * Represents both memory requests and responses.
     */
    class Request
    {
    public:
        typedef uint64_t id_t;    /*!< Request ID type */
        typedef uint32_t flags_t; /*!< Flag type */

        /**
         * Commands and responses possible with a Request object
         */
        typedef enum {
            Read,         /*!< Issue a Read from Memory */
            Write,        /*!< Issue a Write to Memory */
            ReadResp,     /*!< Response from Memory to a Read */
            WriteResp,    /*!< Response from Memory to a Write */
            FlushLine,    /*!< Cache flush request - writeback specified line throughout memory system */
            FlushLineInv, /*!< Cache flush request - writeback and invalidate specified line throughout memory system */
            FlushLineResp, /*!< Response to FlushLine; flag F_FLUSH_SUCCESS indicates success or failure */
            Inv,           /*!< Notification of L1 cache invalidation to core */
            TxBegin,       /*!< Start a new transaction */
            TxEnd,         /*!< End the current lowest transaction */
            TxResp,
            TxAbort,
            TxCommit,
            CustomCmd /*!< Custom memory command: Must also set custCmd opcode */
        } Command;

        /**
         * Flags to specify conditions on a Request
         */
        typedef enum {
            F_NONCACHEABLE = 1 << 1, /*!< This request should not be cached */
            F_LOCKED    = 1 << 2, /*!< This request should be locked.  A LOCKED read should be soon followed by a LOCKED
                                     write (to unlock) */
            F_LLSC      = 1 << 3,
            F_LLSC_RESP = 1 << 4,
            F_FLUSH_SUCCESS =
                1 << 5, /*!< This flag is set if the flush was successful. Flush may fail due to LOCKED lines */
            F_TRANSACTION = 1 << 6
        } Flags;

        /** Type of the payload or data */
        typedef std::vector<uint8_t> dataVec;

        Command           cmd;   /*!< Command to issue */
        std::vector<Addr> addrs; /*!< Target address(es) */
        Addr    addr;  /*!< Target address - DEPRECATED but included for backward compatibility, defaults to addrs[0] */
        size_t  size;  /*!< Size of this request or response */
        dataVec data;  /*!< Payload data (for Write, or ReadResp) */
        flags_t flags; /*!< Flags associated with this request or response */
        flags_t
             memFlags; /*!< Memory flags - ignored by caches except to be passed through with request to main memory */
        id_t id;       /*!< Unique ID to identify responses with requests */
        Addr instrPtr; /*!< Instruction pointer associated with the operation */
        Addr virtualAddr; /*!< Virtual address associated with the operation */
        uint32_t custOpc; /*!< Custom command opcode for CustomdCmd type commands */

        /** Constructor */
        Request(Command cmd, Addr addr, size_t size, dataVec& data, flags_t flags = 0, flags_t memFlags = 0) :
            cmd(cmd),
            addr(addr),
            size(size),
            data(data),
            flags(flags),
            memFlags(memFlags),
            instrPtr(0),
            virtualAddr(0),
            custOpc(0xFFFF)
        {
            addrs.push_back(addr);
            id = main_id++;
        }

        /** Constructor */
        Request(Command cmd, Addr addr, size_t size, flags_t flags = 0, flags_t memFlags = 0) :
            cmd(cmd),
            addr(addr),
            size(size),
            flags(flags),
            memFlags(memFlags),
            instrPtr(0),
            virtualAddr(0),
            custOpc(0xFFFF)
        {
            addrs.push_back(addr);
            id = main_id++;
        }

        /** Constructor */
        Request(
            Command cmd, Addr addr, size_t size, dataVec& data, uint32_t Opc, flags_t flags = 0, flags_t memFlags = 0) :
            cmd(cmd),
            addr(addr),
            size(size),
            data(data),
            flags(flags),
            memFlags(memFlags),
            instrPtr(0),
            virtualAddr(0),
            custOpc(Opc)
        {
            addrs.push_back(addr);
            id = main_id++;
        }

        /** Constructor */
        Request(Command cmd, Addr addr, size_t size, uint32_t Opc, flags_t flags = 0, flags_t memFlags = 0) :
            cmd(cmd),
            addr(addr),
            size(size),
            flags(flags),
            memFlags(memFlags),
            instrPtr(0),
            virtualAddr(0),
            custOpc(Opc)
        {
            addrs.push_back(addr);
            id = main_id++;
        }

        void addAddress(Addr addr) { addrs.push_back(addr); }

        /**
         * @param[in] data_in
         * @brief Set the contents of the payload / data field.
         */
        void setPayload(const std::vector<uint8_t>& data_in) { data = data_in; }

        /**
         * @param[in] data_in
         * @brief Set the contents of the payload / data field.
         */
        void setPayload(uint8_t* data_in, size_t len)
        {
            data.resize(len);
            for ( size_t i = 0; i < len; i++ ) {
                data[i] = data_in[i];
            }
        }

        /**
         * @param[in] newVA
         * @brief  Set the virtual address associated with the operation
         */
        void setVirtualAddress(const Addr newVA) { virtualAddr = newVA; }

        /**
         * @returns the virtual address associated with the operation
         */
        uint64_t getVirtualAddress() { return (uint64_t)virtualAddr; }

        /**
         * @param[in] newIP
         * @brief Sets the instruction pointer associated with the operation
         */
        void setInstructionPointer(const Addr newIP) { instrPtr = newIP; }

        /**
         * @returns the instruction pointer associated with the operation
         */
        Addr getInstructionPointer() { return instrPtr; }

        /**
         * @brief Clears the flags associated with the operation
         */
        void clearFlags(void) { flags = 0; }

        /**
         * @param[in] inValue  Should be one of the flags beginning with F_ in simpleMem
         */
        void setFlags(flags_t inValue) { flags = flags | inValue; }

        /**
         * @returns the flags associated with the operation
         */
        flags_t getFlags(void) { return flags; }

        /**
         * @brief Clears the memory flags associated with the operation
         */
        void clearMemFlags(void) { memFlags = 0; }

        /**
         * @param[in] inValue  Should be one of the flags beginning with F_ in simpleMem
         */
        void setMemFlags(flags_t inValue) { memFlags = memFlags | inValue; }

        /**
         * @returns the memory flags associated with the operation
         */
        flags_t getMemFlags(void) { return memFlags; }

        /**
         * @returns the custom opcode for custom request types
         */
        uint32_t getCustomOpc(void) { return custOpc; }

    private:
        static std::atomic<id_t> main_id;
    };

    /** Constructor, designed to be used via 'loadUserSubComponent and loadAnonymousSubComponent'. */
    SimpleMem(SST::ComponentId_t id, Params& UNUSED(params)) : SubComponent(id) {}

    /** Second half of building the interface.
     * Initialize with link name name, and handler, if any
     * @return true if the link was able to be configured.
     */
    virtual bool initialize(const std::string& linkName, HandlerBase* handler = nullptr) = 0;

    /**
     * Sends a memory-based request during the init() phase
     */
    virtual void sendInitData(Request* req) = 0;

    /**
     * Sends a generic Event during the init() phase
     * (Mostly acts as a passthrough)
     * @see SST::Link::sendInitData()
     */
    virtual void sendInitData(SST::Event* ev) { getLink()->sendInitData(ev); }

    /**
     * Receive any data during the init() phase.
     * @see SST::Link::recvInitData()
     */
    virtual SST::Event* recvInitData() { return getLink()->recvInitData(); }

    /**
     * Returns a handle to the underlying SST::Link
     */
    virtual SST::Link* getLink(void) const = 0;

    /**
     * Send a Request to the other side of the link.
     */
    virtual void sendRequest(Request* req) = 0;

    /**
     * Receive a Request response from the side of the link.
     *
     * Use this method for polling-based applications.
     * Register a handler for push-based notification of responses.
     *
     * @return nullptr if nothing is available.
     * @return Pointer to a Request response (that should be deleted)
     */
    virtual Request* recvResponse(void) = 0;

    /**
     * Get cache/memory line size from the memory system
     *
     * The memory system should provide this and it should be
     * valid after the init() phase is complete, so processors
     * can call this function during setup().
     *
     * For backward compatibiity, the function returns 0 by default.
     * Eventually, interfaces will be required to implement this
     * function.
     *
     * @return 0 if the interface does not provide this capability
     * @return line size of the memory system
     */
    virtual Addr getLineSize() { return 0; }
};

} // namespace Interfaces
} // namespace SST

#endif // SST_CORE_INTERFACES_SIMPLEMEM_H
