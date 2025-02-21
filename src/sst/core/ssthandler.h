// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SSTHANDLER_H
#define SST_CORE_SSTHANDLER_H

#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"

namespace SST {

class Params;

// This file contains base classes for use as various handlers (object
// encapsulating callback functions) in SST.  These handlers are
// checkpointable and encapsulate a pointer to an object and a pointer
// to a member function of the object's class (captured via template
// parameter).  The classes also allow you to optionally add one
// additional piece of static data to be passed into the callback
// function along with any data provided by the caller.  There are two
// versions of this class, one that has no data passed from the caller
// (ending with SSTHandlerNoArgs), and one that has a single item
// passed from the caller (SSTHandler).

// NOTE: Legacy SSTHandler objects are not checkpointable and have
// been deprecated.  Support for legacy handlers will be removed in
// SST 15.

// These classes provide the full functionality of the handlers and
// can be added to a class with the "using" keyword, as follows (a
// class can use any type name they'd like in place of HandlerBase and
// Handler, though those names are preferred for consistency):

// Note: Until support for legacy handlers are removed, the new-style
// handlers should use Handler2Base and Handler2 as the preferred
// names.  After legacy support is removed with SST 15, both Handler
// and Handler2 should both point to the new style handlers.  Handler2
// will be deprecated from SST 15 until SST 16, when Handler will be
// the approved name.

// LEGACY STYLE

// using HandlerBase = SSTHandlerBase<return_type_of_callback, arg_type_of_callback>;

// template <typename classT, typename dataT = void>
// using Handler = SSTHandler<return_type_of_callback, arg_type_of_callback, classT, dataT>;

// Or:

// using HandlerBase = SSTHandlerBaseNoArgs<return_type_of_callback>;

// template <return_type_of_callback, typename classT, typename dataT = void>
// using Handler = SSTHandlerNoArgs<return_type_of_callback, classT, dataT>;

// The handlers are then instanced as follows:

// new Class::Handler<Class>(this, &Class::callback_function)

// Or:

// new Class::Handler<Class,int>(this, &Class::callback_function, 1)


// NEW STYLE
// Note: New style SSTHandler2 uses the same class for functions with and without parameters.

// Note: HandlerBase is the same both both legacy and new style
// using HandlerBase = SSTHandlerBase<return_type_of_callback, arg_type_of_callback>;

// template <typename classT, auto funcT, typename dataT = void>
// using Handler2 = SSTHandler2<return_type_of_callback, arg_type_of_callback, classT, dataT, funcT>;


// The handlers are then instanced as follows:

// new Class::Handler2<Class, &Class::callback_function>(this)

// Or:

// new Class::Handler2<Class, &Class::callback_function, int>(this, 1)


/**********************************************************************
 * Base class templates for handlers.  The base functionlity includes
 * the common API for all handlers.  It also includes management of
 * attach points.
 *
 * There are 4 total expansions of the template across 2 classes,
 * based on whether their return and arg values are void or non-void.
 * Each of these also define the appropriate Attach and/or Intercept
 * points.
 **********************************************************************/

/**
   Base template for handlers which take a class defined argument.

   This default expansion covers the case of non-void return
   type. This version does not support intercepts.
*/
template <typename returnT, typename argT>
class SSTHandlerBase : public SST::Core::Serialization::serializable
{
public:
    /**
       Attach Point to get notified when a handler starts and stops.
       This class is used in conjuction with a Tool type base class to
       create various tool types to attach to the handler.
    */
    class AttachPoint
    {
    public:
        AttachPoint() {}
        virtual ~AttachPoint() {}

        /**
           Function that will be called when a handler is registered
           with the tool implementing the attach point.  The metadata
           passed in will be dependent on what type of tool this is
           attached to.  The uintptr_t returned from this function
           will be passed into the beforeHandler() and afterHandler()
           functions.

           @param mdata Metadata to be passed into the tool

           @return Opaque key that will be passed back into
           beforeHandler() and afterHandler() to identify the source
           of the calls
        */
        virtual uintptr_t registerHandler(const AttachPointMetaData& mdata) = 0;

        /**
           Function to be called before the handler is called.

           @key uintptr_t returned from registerHandler() when handler
           was registered with the tool

           @arg argument that will be passed to the handler function.
           If argT is a pointer, this will be passed as a const
           pointer, if not, it will be passed as a const reference
        */
        virtual void beforeHandler(
            uintptr_t                                                                                    key,
            std::conditional_t<std::is_pointer_v<argT>, const std::remove_pointer_t<argT>*, const argT&> arg) = 0;

        /**
           Function to be called after the handler is called.  The key
           passed in is the uintptr_t returned from registerHandler()

           @param key uintptr_t returned from registerHandler() when
           handler was registered with the tool

           @param ret_value value that was returned by the handler. If
           retunT is a pointer, this will be passed as a const
           pointer, if not, it will be passed as a const reference
        */
        virtual void afterHandler(
            uintptr_t key,
            std::conditional_t<std::is_pointer_v<returnT>, const std::remove_pointer_t<returnT>*, const returnT&>
                ret_value) = 0;

        /**
           Function that will be called to handle the key returned
           from registerHandler, if the AttachPoint tool is
           serializable.  This is needed because the key is opaque to
           the Link, so it doesn't know how to handle it during
           serialization.  During SIZE and PACK phases of
           serialization, the tool needs to store out any information
           that will be needed to recreate data that is reliant on the
           key.  On UNPACK, the function needs to recreate any state
           and reinitialize the passed in key reference to the proper
           state to continue to make valid calls to beforeHandler()
           and afterHandler().

           Since not all tools will be serializable, there is a
           default, empty implementation.

           @param ser Serializer to use for serialization

           @param key Key that would be passed into the
           beforeHandler() and afterHandler() functions.
         */
        virtual void
        serializeHandlerAttachPointKey(SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& UNUSED(key))
        {}
    };

private:
    using ToolList           = std::vector<std::pair<AttachPoint*, uintptr_t>>;
    ToolList* attached_tools = nullptr;

protected:
    // Implementation of operator() to be done in child classes
    virtual returnT operator_impl(argT) = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        switch ( ser.mode() ) {
        case Core::Serialization::serializer::SIZER:
        case Core::Serialization::serializer::PACK:
        {
            ToolList tools;
            if ( attached_tools ) {
                for ( auto x : *attached_tools ) {
                    if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) { tools.push_back(x); }
                }
            }
            size_t tool_count = tools.size();
            ser&   tool_count;
            if ( tool_count > 0 ) {
                // Serialize each tool, then call
                // serializeEventAttachPointKey() to serialize any
                // data associated with the key
                for ( auto x : tools ) {
                    SST::Core::Serialization::serializable* obj =
                        dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                    ser& obj;
                    x.first->serializeHandlerAttachPointKey(ser, x.second);
                }
            }
            break;
        }
        case Core::Serialization::serializer::UNPACK:
        {
            size_t tool_count;
            ser&   tool_count;
            if ( tool_count > 0 ) {
                attached_tools = new ToolList();
                for ( size_t i = 0; i < tool_count; ++i ) {
                    SST::Core::Serialization::serializable* tool;
                    uintptr_t                               key;
                    ser&                                    tool;
                    AttachPoint*                            ap = dynamic_cast<AttachPoint*>(tool);
                    ap->serializeHandlerAttachPointKey(ser, key);
                    attached_tools->emplace_back(ap, key);
                }
            }
            else {
                attached_tools = nullptr;
            }
            break;
        }
        default:
            break;
        }
    }

public:
    virtual ~SSTHandlerBase() {}

    inline returnT operator()(argT arg)
    {
        if ( !attached_tools ) return operator_impl(arg);

        // Tools attached
        for ( auto& x : *attached_tools )
            x.first->beforeHandler(x.second, arg);

        returnT ret = operator_impl(arg);

        for ( auto& x : *attached_tools )
            x.first->afterHandler(x.second, ret);

        return ret;
    }

    /**
       Attaches a tool to the AttachPoint

       @param tool Tool to attach

       @param mdata Metadata to pass to the tool
    */
    void attachTool(AttachPoint* tool, const AttachPointMetaData& mdata)
    {
        if ( !attached_tools ) attached_tools = new ToolList();

        auto key = tool->registerHandler(mdata);
        attached_tools->push_back(std::make_pair(tool, key));
    }

    /**
       Transfers attached tools from existing handler
     */
    void transferAttachedToolInfo(SSTHandlerBase* handler)
    {
        if ( handler->attached_tools ) {
            attached_tools          = handler->attached_tools;
            handler->attached_tools = nullptr;
        }
    }

private:
    ImplementVirtualSerializable(SSTHandlerBase)
};


/**
   Base template for handlers which take an class defined argument.

   This expansion covers the case of void return type. This version
   supports intercepts.
*/
template <typename argT>
class SSTHandlerBase<void, argT> : public SST::Core::Serialization::serializable
{
public:
    /**
       Attach Point to get notified when a handler starts and stops.
       This class is used in conjuction with a Tool type base class to
       create various tool types to attach to the handler.
    */
    class AttachPoint
    {
    public:
        AttachPoint() {}
        virtual ~AttachPoint() {}

        /**
           Function that will be called when a handler is registered
           with the tool implementing the attach point.  The metadata
           passed in will be dependent on what type of tool this is
           attached to.  The uintptr_t returned from this function
           will be passed into the beforeHandler() and afterHandler()
           functions.

           @param mdata Metadata to be passed into the tool

           @return Opaque key that will be passed back into
           beforeHandler() and afterHandler() to identify the source
           of the calls
        */
        virtual uintptr_t registerHandler(const AttachPointMetaData& mdata) = 0;

        /**
           Function to be called before the handler is called.

           @key uintptr_t returned from registerHandler() when handler
           was registered with the tool

           @arg argument that will be passed to the handler function.
           If argT is a pointer, this will be passed as a const
           pointer, if not, it will be passed as a const reference
        */
        virtual void beforeHandler(
            uintptr_t                                                                                    key,
            std::conditional_t<std::is_pointer_v<argT>, const std::remove_pointer_t<argT>*, const argT&> arg) = 0;

        /**
           Function to be called after the handler is called.  The key
           passed in is the uintptr_t returned from registerHandler()

           @param key uintptr_t returned from registerHandler() when
           handler was registered with the tool
        */
        virtual void afterHandler(uintptr_t key) = 0;

        /**
           Function that will be called to handle the key returned
           from registerHandler, if the AttachPoint tool is
           serializable.  This is needed because the key is opaque to
           the Link, so it doesn't know how to handle it during
           serialization.  During SIZE and PACK phases of
           serialization, the tool needs to store out any information
           that will be needed to recreate data that is reliant on the
           key.  On UNPACK, the function needs to recreate any state
           and reinitialize the passed in key reference to the proper
           state to continue to make valid calls to beforeHandler()
           and afterHandler().

           Since not all tools will be serializable, there is a
           default, empty implementation.

           @param ser Serializer to use for serialization

           @param key Key that would be passed into the
           beforeHandler() and afterHandler() functions.
         */
        virtual void
        serializeHandlerAttachPointKey(SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& UNUSED(key))
        {}
    };


    /**
       Attach Point to intercept the data being delivered by a
       Handler.  Class is not usable for Handlers that don't take a
       parameter and/or return a value
    */
    class InterceptPoint
    {
    public:
        /**
           Function that will be called when a handler is registered
           with the tool implementing the intercept attach point.  The
           metadata passed in will be dependent on what type of tool
           this is attached to.  The uintptr_t returned from this
           function will be passed into the intercept() function.

           @param mdata Metadata to be passed into the tool

           @return Opaque key that will be passed back into
           interceptHandler() calls
        */
        virtual uintptr_t registerHandlerIntercept(const AttachPointMetaData& mdata) = 0;

        /**
           Function that will be called before the event handler to
           let the attach point intercept the data.  The data can be
           modified, and if cancel is set to true, the handler will
           not be executed.  If cancel is set to true and the
           ownership of a pointer is passed by the call to the
           handler, then the function should also delete the data.

           @param key Key that was returned from
           registerHandlerIntercept() function

           @param data Data that is to be passed to the handler

           @param[out] cancel Set to true if the handler delivery
           should be cancelled.
        */
        virtual void interceptHandler(uintptr_t key, argT& data, bool& cancel) = 0;

        /**
           Function that will be called to handle the key returned
           from registerHandlerIntercept, if the AttachPoint tool is
           serializable.  This is needed because the key is opaque to
           the Link, so it doesn't know how to handle it during
           serialization.  During SIZE and PACK phases of
           serialization, the tool needs to store out any information
           that will be needed to recreate data that is reliant on the
           key.  On UNPACK, the function needs to recreate any state
           and reinitialize the passed in key reference to the proper
           state to continue to make valid calls to
           interceptHandler().

           Since not all tools will be serializable, there is a
           default, empty implementation.

           @param ser Serializer to use for serialization

           @param key Key that would be passed into the interceptHandler() function.
         */
        virtual void
        serializeHandlerInterceptPointKey(SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& UNUSED(key))
        {}
    };

private:
    struct ToolList
    {
        std::vector<std::pair<AttachPoint*, uintptr_t>>    attach_tools;
        std::vector<std::pair<InterceptPoint*, uintptr_t>> intercept_tools;
    };
    ToolList* attached_tools = nullptr;

protected:
    // Implementation of operator() to be done in child classes
    virtual void operator_impl(argT) = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        switch ( ser.mode() ) {
        case Core::Serialization::serializer::SIZER:
        case Core::Serialization::serializer::PACK:
        {
            ToolList tools;
            if ( attached_tools ) {
                for ( auto x : attached_tools->attach_tools ) {
                    if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) {
                        tools.attach_tools.push_back(x);
                    }
                }
                for ( auto x : attached_tools->intercept_tools ) {
                    if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) {
                        tools.intercept_tools.push_back(x);
                    }
                }
            }

            // Serialize AttachPoint tools
            size_t tool_count = tools.attach_tools.size();
            ser&   tool_count;
            if ( tool_count > 0 ) {
                // Serialize each tool, then call
                // serializeEventAttachPointKey() to serialize any
                // data associated with the key
                for ( auto x : tools.attach_tools ) {
                    SST::Core::Serialization::serializable* obj =
                        dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                    ser& obj;
                    x.first->serializeHandlerAttachPointKey(ser, x.second);
                }
            }
            // Serialize InterceptPoint tools
            tool_count = tools.intercept_tools.size();
            ser& tool_count;
            if ( tool_count > 0 ) {
                // Serialize each tool, then call
                // serializeEventAttachPointKey() to serialize any
                // data associated with the key
                for ( auto x : tools.intercept_tools ) {
                    SST::Core::Serialization::serializable* obj =
                        dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                    ser& obj;
                    x.first->serializeHandlerInterceptPointKey(ser, x.second);
                }
            }
            break;
        }
        case Core::Serialization::serializer::UNPACK:
        {
            // Get serialized AttachPoint tools
            size_t tool_count;
            ser&   tool_count;
            if ( tool_count > 0 ) {
                attached_tools = new ToolList();
                for ( size_t i = 0; i < tool_count; ++i ) {
                    SST::Core::Serialization::serializable* tool;
                    uintptr_t                               key;
                    ser&                                    tool;
                    AttachPoint*                            ap = dynamic_cast<AttachPoint*>(tool);
                    ap->serializeHandlerAttachPointKey(ser, key);
                    attached_tools->attach_tools.emplace_back(ap, key);
                }
            }
            else {
                attached_tools = nullptr;
            }

            // Get serialized InterceptPoint tools
            ser& tool_count;
            if ( tool_count > 0 ) {
                if ( nullptr == attached_tools ) attached_tools = new ToolList();
                for ( size_t i = 0; i < tool_count; ++i ) {
                    SST::Core::Serialization::serializable* tool;
                    uintptr_t                               key;
                    ser&                                    tool;
                    InterceptPoint*                         ip = dynamic_cast<InterceptPoint*>(tool);
                    ip->serializeHandlerInterceptPointKey(ser, key);
                    attached_tools->intercept_tools.emplace_back(ip, key);
                }
            }
            break;
        }
        default:
            break;
        }
    }

public:
    virtual ~SSTHandlerBase() {}

    inline void operator()(argT arg)
    {
        if ( !attached_tools ) return operator_impl(arg);

        // Tools attached
        for ( auto& x : attached_tools->attach_tools )
            x.first->beforeHandler(x.second, arg);

        // Check any intercepts

        bool cancel = false;
        for ( auto& x : attached_tools->intercept_tools ) {
            x.first->interceptHandler(x.second, arg, cancel);
            if ( cancel ) {
                // Handler cancelled; need to break since arg may
                // no longer be valid and no other intercepts
                // should be called
                break;
            }
        }
        if ( !cancel ) { operator_impl(arg); }

        for ( auto& x : attached_tools->attach_tools )
            x.first->afterHandler(x.second);

        return;
    }


    /**
       Attaches a tool to the AttachPoint

       @param tool Tool to attach

       @param mdata Metadata to pass to the tool
    */
    void attachTool(AttachPoint* tool, const AttachPointMetaData& mdata)
    {
        if ( !attached_tools ) attached_tools = new ToolList();

        auto key = tool->registerHandler(mdata);
        attached_tools->attach_tools.push_back(std::make_pair(tool, key));
    }

    /**
       Attaches a tool to the AttachPoint

       @param tool Tool to attach

       @param mdata Metadata to pass to the tool
    */
    void attachInterceptTool(InterceptPoint* tool, const AttachPointMetaData& mdata)
    {
        if ( !attached_tools ) attached_tools = new ToolList();

        auto key = tool->registerHandlerIntercept(mdata);
        attached_tools->intercept_tools.push_back(std::make_pair(tool, key));
    }

    /**
       Transfers attached tools from existing handler
     */
    void transferAttachedToolInfo(SSTHandlerBase* handler)
    {
        if ( handler->attached_tools ) {
            attached_tools          = handler->attached_tools;
            handler->attached_tools = nullptr;
        }
    }

private:
    ImplementVirtualSerializable(SSTHandlerBase)
};


/**
   Base template for handlers which don't take a class defined
   argument.

   This expansion covers the case of non-void return type. This
   version does not support intercepts.
*/
template <typename returnT>
class SSTHandlerBase<returnT, void> : public SST::Core::Serialization::serializable
{
public:
    /**
       Attach Point to get notified when a handler starts and stops.
       This class is used in conjuction with a Tool type base class to
       create various tool types to attach to the handler.
    */
    class AttachPoint
    {
    public:
        AttachPoint() {}
        virtual ~AttachPoint() {}

        /**
           Function that will be called when a handler is registered
           with the tool implementing the attach point.  The metadata
           passed in will be dependent on what type of tool this is
           attached to.  The uintptr_t returned from this function
           will be passed into the beforeHandler() and afterHandler()
           functions.

           @param mdata Metadata to be passed into the tool

           @return Opaque key that will be passed back into
           beforeHandler() and afterHandler() to identify the source
           of the calls
        */
        virtual uintptr_t registerHandler(const AttachPointMetaData& mdata) = 0;

        /**
           Function to be called before the handler is called.

           @key uintptr_t returned from registerHandler() when handler
           was registered with the tool
        */
        virtual void beforeHandler(uintptr_t key) = 0;

        /**
           Function to be called after the handler is called.  The key
           passed in is the uintptr_t returned from registerHandler()

           @param key uintptr_t returned from registerHandler() when
           handler was registered with the tool

           @param ret_value value that was returned by the handler. If
           retunT is a pointer, this will be passed as a const
           pointer, if not, it will be passed as a const reference
        */
        virtual void afterHandler(
            uintptr_t key,
            std::conditional_t<std::is_pointer_v<returnT>, const std::remove_pointer_t<returnT>*, const returnT&>
                ret_value) = 0;

        /**
           Function that will be called to handle the key returned
           from registerHandler, if the AttachPoint tool is
           serializable.  This is needed because the key is opaque to
           the Link, so it doesn't know how to handle it during
           serialization.  During SIZE and PACK phases of
           serialization, the tool needs to store out any information
           that will be needed to recreate data that is reliant on the
           key.  On UNPACK, the function needs to recreate any state
           and reinitialize the passed in key reference to the proper
           state to continue to make valid calls to beforeHandler()
           and afterHandler().

           Since not all tools will be serializable, there is a
           default, empty implementation.

           @param ser Serializer to use for serialization

           @param key Key that would be passed into the
           beforeHandler() and afterHandler() functions.
         */
        virtual void
        serializeHandlerAttachPointKey(SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& UNUSED(key))
        {}
    };

private:
    using ToolList           = std::vector<std::pair<AttachPoint*, uintptr_t>>;
    ToolList* attached_tools = nullptr;

protected:
    // Implementation of operator() to be done in child classes
    virtual returnT operator_impl() = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        switch ( ser.mode() ) {
        case Core::Serialization::serializer::SIZER:
        case Core::Serialization::serializer::PACK:
        {
            ToolList tools;
            if ( attached_tools ) {
                for ( auto x : *attached_tools ) {
                    if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) { tools.push_back(x); }
                }
            }
            size_t tool_count = tools.size();
            ser&   tool_count;
            if ( tool_count > 0 ) {
                // Serialize each tool, then call
                // serializeEventAttachPointKey() to serialize any
                // data associated with the key
                for ( auto x : tools ) {
                    SST::Core::Serialization::serializable* obj =
                        dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                    ser& obj;
                    x.first->serializeHandlerAttachPointKey(ser, x.second);
                }
            }
            break;
        }
        case Core::Serialization::serializer::UNPACK:
        {
            size_t tool_count;
            ser&   tool_count;
            if ( tool_count > 0 ) {
                attached_tools = new ToolList();
                for ( size_t i = 0; i < tool_count; ++i ) {
                    SST::Core::Serialization::serializable* tool;
                    uintptr_t                               key;
                    ser&                                    tool;
                    AttachPoint*                            ap = dynamic_cast<AttachPoint*>(tool);
                    ap->serializeHandlerAttachPointKey(ser, key);
                    attached_tools->emplace_back(ap, key);
                }
            }
            else {
                attached_tools = nullptr;
            }
            break;
        }
        default:
            break;
        }
    }


public:
    virtual ~SSTHandlerBase() {}

    inline returnT operator()()
    {
        if ( attached_tools ) {
            for ( auto& x : *attached_tools )
                x.first->beforeHandler(x.second);

            returnT ret = operator_impl();

            for ( auto& x : *attached_tools )
                x.first->afterHandler(x.second, ret);

            return ret;
        }
        return operator_impl();
    }

    /**
       Attaches a tool to the AttachPoint

       @param tool Tool to attach

       @param mdata Metadata to pass to the tool
    */
    void attachTool(AttachPoint* tool, const AttachPointMetaData& mdata)
    {
        if ( !attached_tools ) attached_tools = new ToolList();

        auto key = tool->registerHandler(mdata);
        attached_tools->push_back(std::make_pair(tool, key));
    }

    /**
       Transfers attached tools from existing handler
     */
    void transferAttachedToolInfo(SSTHandlerBase* handler)
    {
        if ( handler->attached_tools ) {
            attached_tools          = handler->attached_tools;
            handler->attached_tools = nullptr;
        }
    }

private:
    ImplementVirtualSerializable(SSTHandlerBase)
};


/**
   Base template for handlers which don't take a class defined
   argument.

   This expansion covers the case of void return type. This version
   does not support intercepts.
*/
template <>
class SSTHandlerBase<void, void> : public SST::Core::Serialization::serializable
{
public:
    /**
       Attach Point to get notified when a handler starts and stops.
       This class is used in conjuction with a Tool type base class to
       create various tool types to attach to the handler.
    */
    class AttachPoint
    {
    public:
        AttachPoint() {}
        virtual ~AttachPoint() {}

        /**
           Function that will be called when a handler is registered
           with the tool implementing the attach point.  The metadata
           passed in will be dependent on what type of tool this is
           attached to.  The uintptr_t returned from this function
           will be passed into the beforeHandler() and afterHandler()
           functions.

           @param mdata Metadata to be passed into the tool

           @return Opaque key that will be passed back into
           beforeHandler() and afterHandler() to identify the source
           of the calls
        */
        virtual uintptr_t registerHandler(const AttachPointMetaData& mdata) = 0;

        /**
           Function to be called before the handler is called.

           @key uintptr_t returned from registerHandler() when handler
           was registered with the tool
        */
        virtual void beforeHandler(uintptr_t key) = 0;

        /**
           Function to be called after the handler is called.  The key
           passed in is the uintptr_t returned from registerHandler()

           @param key uintptr_t returned from registerHandler() when
           handler was registered with the tool
        */
        virtual void afterHandler(uintptr_t key) = 0;

        /**
           Function that will be called to handle the key returned
           from registerHandler, if the AttachPoint tool is
           serializable.  This is needed because the key is opaque to
           the Link, so it doesn't know how to handle it during
           serialization.  During SIZE and PACK phases of
           serialization, the tool needs to store out any information
           that will be needed to recreate data that is reliant on the
           key.  On UNPACK, the function needs to recreate any state
           and reinitialize the passed in key reference to the proper
           state to continue to make valid calls to beforeHandler()
           and afterHandler().

           Since not all tools will be serializable, there is a
           default, empty implementation.

           @param ser Serializer to use for serialization

           @param key Key that would be passed into the
           beforeHandler() and afterHandler() functions.
         */
        virtual void
        serializeHandlerAttachPointKey(SST::Core::Serialization::serializer& UNUSED(ser), uintptr_t& UNUSED(key))
        {}
    };

private:
    using ToolList           = std::vector<std::pair<AttachPoint*, uintptr_t>>;
    ToolList* attached_tools = nullptr;

protected:
    // Implementation of operator() to be done in child classes
    virtual void operator_impl() = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        switch ( ser.mode() ) {
        case Core::Serialization::serializer::SIZER:
        case Core::Serialization::serializer::PACK:
        {
            ToolList tools;
            if ( attached_tools ) {
                for ( auto x : *attached_tools ) {
                    if ( dynamic_cast<SST::Core::Serialization::serializable*>(x.first) ) { tools.push_back(x); }
                }
            }
            size_t tool_count = tools.size();
            ser&   tool_count;
            if ( tool_count > 0 ) {
                // Serialize each tool, then call
                // serializeEventAttachPointKey() to serialize any
                // data associated with the key
                for ( auto x : tools ) {
                    SST::Core::Serialization::serializable* obj =
                        dynamic_cast<SST::Core::Serialization::serializable*>(x.first);
                    ser& obj;
                    x.first->serializeHandlerAttachPointKey(ser, x.second);
                }
            }
            break;
        }
        case Core::Serialization::serializer::UNPACK:
        {
            size_t tool_count;
            ser&   tool_count;
            if ( tool_count > 0 ) {
                attached_tools = new ToolList();
                for ( size_t i = 0; i < tool_count; ++i ) {
                    SST::Core::Serialization::serializable* tool;
                    uintptr_t                               key;
                    ser&                                    tool;
                    AttachPoint*                            ap = dynamic_cast<AttachPoint*>(tool);
                    ap->serializeHandlerAttachPointKey(ser, key);
                    attached_tools->emplace_back(ap, key);
                }
            }
            else {
                attached_tools = nullptr;
            }
            break;
        }
        default:
            break;
        }
    }

public:
    virtual ~SSTHandlerBase() {}

    inline void operator()()
    {
        if ( attached_tools ) {
            for ( auto& x : *attached_tools )
                x.first->beforeHandler(x.second);

            operator_impl();

            for ( auto& x : *attached_tools )
                x.first->afterHandler(x.second);

            return;
        }
        return operator_impl();
    }

    /**
       Attaches a tool to the AttachPoint

       @param tool Tool to attach

       @param mdata Metadata to pass to the tool
    */
    void attachTool(AttachPoint* tool, const AttachPointMetaData& mdata)
    {
        if ( !attached_tools ) attached_tools = new ToolList();

        auto key = tool->registerHandler(mdata);
        attached_tools->push_back(std::make_pair(tool, key));
    }

    /**
       Transfers attached tools from existing handler
     */
    void transferAttachedToolInfo(SSTHandlerBase* handler)
    {
        if ( handler->attached_tools ) {
            attached_tools          = handler->attached_tools;
            handler->attached_tools = nullptr;
        }
    }

private:
    ImplementVirtualSerializable(SSTHandlerBase)
};


/**********************************************************************
 * Legacy Handlers
 *
 * These handlers do not support checkpointing and will be removed in
 * SST 15.
 **********************************************************************/

template <typename returnT>
using SSTHandlerBaseNoArgs = SSTHandlerBase<returnT, void>;
/**
 * Handler class with user-data argument
 */
template <typename returnT, typename argT, typename classT, typename dataT = void>
class SSTHandler : public SSTHandlerBase<returnT, argT>
{
private:
    typedef returnT (classT::*PtrMember)(argT, dataT);
    classT*         object;
    const PtrMember member;
    dataT           data;

public:
    /** Constructor
     * @param object - Pointer to Object upon which to call the handler
     * @param member - Member function to call as the handler
     * @param data - Additional argument to pass to handler
     */
    SSTHandler(classT* const object, PtrMember member, dataT data) :
        SSTHandlerBase<returnT, argT>(),
        object(object),
        member(member),
        data(data)
    {}

    returnT operator_impl(argT arg) override { return (object->*member)(arg, data); }

    NotSerializable(SSTHandler)
};


/**
 * Event Handler class with no user-data.
 */
template <typename returnT, typename argT, typename classT>
class SSTHandler<returnT, argT, classT, void> : public SSTHandlerBase<returnT, argT>
{
private:
    typedef returnT (classT::*PtrMember)(argT);
    const PtrMember member;
    classT*         object;

public:
    /** Constructor
     * @param object - Pointer to Object upon which to call the handler
     * @param member - Member function to call as the handler
     */
    SSTHandler(classT* const object, PtrMember member) : SSTHandlerBase<returnT, argT>(), member(member), object(object)
    {}

    returnT operator_impl(argT arg) override { return (object->*member)(arg); }

    NotSerializable(SSTHandler)
};


/**
 * Event Handler class with user-data argument
 */
template <typename returnT, typename classT, typename dataT = void>
class SSTHandlerNoArgs : public SSTHandlerBaseNoArgs<returnT>
{
private:
    typedef returnT (classT::*PtrMember)(dataT);
    classT*         object;
    const PtrMember member;
    dataT           data;

public:
    /** Constructor
     * @param object - Pointer to Object upon which to call the handler
     * @param member - Member function to call as the handler
     * @param data - Additional argument to pass to handler
     */
    SSTHandlerNoArgs(classT* const object, PtrMember member, dataT data) :
        SSTHandlerBase<returnT, void>(),
        object(object),
        member(member),
        data(data)
    {}

    void operator_impl() override { return (object->*member)(data); }

    NotSerializable(SSTHandlerNoArgs)
};


/**
 * Event Handler class with no user-data.
 */
template <typename returnT, typename classT>
class SSTHandlerNoArgs<returnT, classT, void> : public SSTHandlerBaseNoArgs<returnT>
{
private:
    typedef returnT (classT::*PtrMember)();
    const PtrMember member;
    classT*         object;

public:
    /** Constructor
     * @param object - Pointer to Object upon which to call the handler
     * @param member - Member function to call as the handler
     */
    SSTHandlerNoArgs(classT* const object, PtrMember member) :
        SSTHandlerBase<returnT, void>(),
        member(member),
        object(object)
    {}

    void operator_impl() override { return (object->*member)(); }

    NotSerializable(SSTHandlerNoArgs)
};


/**********************************************************************
 * New Style Handlers
 *
 * These handlers support checkpointing
 **********************************************************************/

/**
   Base template for the class.  If this one gets chosen, then there
   is a mismatch somewhere, so it will just static_assert
 */
template <typename returnT, typename argT, typename classT, typename dataT, auto funcT>
class SSTHandler2 : public SSTHandlerBase<returnT, argT>
{
    // This has to be dependent on a template parameter, otherwise it always asserts.
    static_assert((funcT, false), "Mismatched handler templates.");
};


/**
 * Handler class with user-data argument
 */
template <typename returnT, typename argT, typename classT, typename dataT, returnT (classT::*funcT)(argT, dataT)>
class SSTHandler2<returnT, argT, classT, dataT, funcT> : public SSTHandlerBase<returnT, argT>
{
private:
    classT* object;
    dataT   data;

public:
    /** Constructor
     * @param object - Pointer to Object upon which to call the handler
     * @param data - Additional argument to pass to handler
     */
    SSTHandler2(classT* const object, dataT data) : SSTHandlerBase<returnT, argT>(), object(object), data(data) {}

    SSTHandler2() {}

    returnT operator_impl(argT arg) override { return (object->*funcT)(arg, data); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SSTHandlerBase<returnT, argT>::serialize_order(ser);
        ser& object;
        ser& data;
    }

    ImplementSerializable(SSTHandler2)
};


/**
 * Event Handler class with no user-data.
 */
template <typename returnT, typename argT, typename classT, returnT (classT::*funcT)(argT)>
class SSTHandler2<returnT, argT, classT, void, funcT> : public SSTHandlerBase<returnT, argT>
{
private:
    classT* object;

public:
    /** Constructor
     * @param object - Pointer to Object upon which to call the handler
     * @param member - Member function to call as the handler
     */
    SSTHandler2(classT* const object) : SSTHandlerBase<returnT, argT>(), object(object) {}
    SSTHandler2() {}

    returnT operator_impl(argT arg) override { return (object->*funcT)(arg); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SSTHandlerBase<returnT, argT>::serialize_order(ser);
        ser& object;
    }

    ImplementSerializable(SSTHandler2)
};


/**
 * Event Handler class with user-data argument
 */
template <typename returnT, typename classT, typename dataT, returnT (classT::*funcT)(dataT)>
class SSTHandler2<returnT, void, classT, dataT, funcT> : public SSTHandlerBase<returnT, void>
{
private:
    classT* object;
    dataT   data;

public:
    /** Constructor
     * @param object - Pointer to Object upon which to call the handler
     * @param data - Additional argument to pass to handler
     */
    SSTHandler2(classT* const object, dataT data) : SSTHandlerBase<returnT, void>(), object(object), data(data) {}
    SSTHandler2() {}

    returnT operator_impl() override { return (object->*funcT)(data); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SSTHandlerBase<returnT, void>::serialize_order(ser);
        ser& object;
        ser& data;
    }

    ImplementSerializable(SSTHandler2)
};


/**
 * Event Handler class with user-data argument
 */
template <typename returnT, typename classT, returnT (classT::*funcT)()>
class SSTHandler2<returnT, void, classT, void, funcT> : public SSTHandlerBase<returnT, void>
{
private:
    classT* object;

public:
    /** Constructor
     * @param object - Pointer to Object upon which to call the handler
     * @param data - Additional argument to pass to handler
     */
    SSTHandler2(classT* const object) : SSTHandlerBase<returnT, void>(), object(object) {}
    SSTHandler2() {}

    returnT operator_impl() override { return (object->*funcT)(); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SSTHandlerBase<returnT, void>::serialize_order(ser);
        ser& object;
    }

    ImplementSerializable(SSTHandler2)
};

} // namespace SST

#endif // SST_CORE_SSTHANDLER_H
