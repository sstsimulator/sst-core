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

#ifndef SST_CORE_SSTHANDLER_H
#define SST_CORE_SSTHANDLER_H

#include "sst/core/profile/profiletool.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"

namespace SST {

class Params;

/**
   Just a tag class for the various metadata that will need to be
   stored in the simulation object to allow ProfileTools to get the
   data they need.
 */
class HandlerMetaData
{
public:
    virtual ~HandlerMetaData() {}
};


/**
   Base class for Profile tools for Handlers
*/
class HandlerProfileToolAPI : public Profile::ProfileTool
{
public:
    // Register with ELI as base API
    SST_ELI_REGISTER_PROFILETOOL_API(SST::HandlerProfileToolAPI, Params&)

protected:
    HandlerProfileToolAPI(const std::string& name) : Profile::ProfileTool(name) {}
    ~HandlerProfileToolAPI() {}

public:
    virtual uintptr_t registerHandler(const HandlerMetaData& mdata) = 0;

    virtual void handlerStart(uintptr_t UNUSED(key)) {}
    virtual void handlerEnd(uintptr_t UNUSED(key)) {}
};

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
// names.  After legacy support is remoed with SST 15, both Handler
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


/// Functor classes for Event handling

class SSTHandlerBaseProfile : public SST::Core::Serialization::serializable
{
protected:
    // This class will hold the id for the handler and the profiling
    // tools attached to it, if profiling is enabled for this handler.
    class HandlerProfileToolList
    {
        static std::atomic<HandlerId_t> id_counter;
        HandlerId_t                     my_id;

    public:
        HandlerProfileToolList();

        void handlerStart()
        {
            for ( auto& x : tools )
                x.first->handlerStart(x.second);
        }
        void handlerEnd()
        {
            for ( auto& x : tools )
                x.first->handlerEnd(x.second);
        }

        /**
           Adds a profile tool the the list and registers this handler
           with the profile tool
         */
        void addProfileTool(HandlerProfileToolAPI* tool, const HandlerMetaData& mdata)
        {
            auto key = tool->registerHandler(mdata);
            tools.push_back(std::make_pair(tool, key));
        }

        HandlerId_t getId() { return my_id; }


    private:
        std::vector<std::pair<HandlerProfileToolAPI*, uintptr_t>> tools;
    };

    // List of profiling tools attached to this handler
    HandlerProfileToolList* profile_tools;


public:
    SSTHandlerBaseProfile() : profile_tools(nullptr) {}

    virtual ~SSTHandlerBaseProfile()
    {
        if ( profile_tools ) delete profile_tools;
    }

    void addProfileTool(HandlerProfileToolAPI* tool, const HandlerMetaData& mdata)
    {
        if ( !profile_tools ) profile_tools = new HandlerProfileToolList();
        profile_tools->addProfileTool(tool, mdata);
    }

    void transferProfilingInfo(SSTHandlerBaseProfile* handler)
    {
        if ( handler->profile_tools ) {
            profile_tools          = handler->profile_tools;
            handler->profile_tools = nullptr;
        }
    }

    /**
       Get the ID for this Handler.  Handler IDs are only used for
       profiling, so if this function is called, it will also set
       things up to accept ProfileTools.
    */
    HandlerId_t getId()
    {
        if ( !profile_tools ) profile_tools = new HandlerProfileToolList();
        return profile_tools->getId();
    }

    ImplementVirtualSerializable(SSTHandlerBaseProfile)
};


/// Handlers with 1 handler defined argument to callback from caller
template <typename returnT, typename argT>
class SSTHandlerBase : public SSTHandlerBaseProfile
{
    // Implementation of operator() to be done in child classes
    virtual returnT operator_impl(argT) = 0;

public:
    ~SSTHandlerBase() {}

    inline returnT operator()(argT arg)
    {
        if ( profile_tools ) {
            if constexpr ( std::is_void_v<returnT> ) {
                profile_tools->handlerStart();
                operator_impl(arg);
                profile_tools->handlerEnd();
                return;
            }
            else {
                profile_tools->handlerStart();
                auto ret = operator_impl(arg);
                profile_tools->handlerEnd();
                return ret;
            }
        }
        return operator_impl(arg);
    }
    ImplementVirtualSerializable(SSTHandlerBase)
};


/// Handlers with no arguments to callback from caller
template <typename returnT>
class SSTHandlerBase<returnT, void> : public SSTHandlerBaseProfile
{

protected:
    virtual returnT operator_impl() = 0;

public:
    SSTHandlerBase() {}

    /** Handler function */
    virtual ~SSTHandlerBase() {}

    inline returnT operator()()
    {
        if ( profile_tools ) {
            if constexpr ( std::is_void_v<returnT> ) {
                profile_tools->handlerStart();
                operator_impl();
                profile_tools->handlerEnd();
                return;
            }
            else {
                profile_tools->handlerStart();
                auto ret = operator_impl();
                profile_tools->handlerEnd();
                return ret;
            }
        }
        return operator_impl();
    }
    ImplementVirtualSerializable(SSTHandlerBase)
};


/**************************************
 * Legacy Handlers
 **************************************/

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

/// Handlers with no arguments to callback from caller
template <typename returnT>
class SSTHandlerBaseNoArgs : public SSTHandlerBaseProfile
{

protected:
    virtual returnT operator_impl() = 0;

public:
    SSTHandlerBaseNoArgs() {}

    /** Handler function */
    virtual ~SSTHandlerBaseNoArgs() {}

    inline returnT operator()()
    {
        if ( profile_tools ) {
            if constexpr ( std::is_void_v<returnT> ) {
                profile_tools->handlerStart();
                operator_impl();
                profile_tools->handlerEnd();
                return;
            }
            else {
                profile_tools->handlerStart();
                auto ret = operator_impl();
                profile_tools->handlerEnd();
                return ret;
            }
        }
        return operator_impl();
    }
    ImplementVirtualSerializable(SSTHandlerBaseNoArgs)
};


template <>
class SSTHandlerBaseNoArgs<void> : public SSTHandlerBaseProfile
{

protected:
    virtual void operator_impl() = 0;

public:
    SSTHandlerBaseNoArgs() {}

    /** Handler function */
    virtual ~SSTHandlerBaseNoArgs() {}

    inline void operator()()
    {
        if ( profile_tools ) {
            profile_tools->handlerStart();
            operator_impl();
            profile_tools->handlerEnd();
            return;
        }
        return operator_impl();
    }

    ImplementVirtualSerializable(SSTHandlerNoArgs)
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
        SSTHandlerBaseNoArgs<returnT>(),
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
        SSTHandlerBaseNoArgs<returnT>(),
        member(member),
        object(object)
    {}

    void operator_impl() override { return (object->*member)(); }

    NotSerializable(SSTHandlerNoArgs)
};


/**************************************
 * New style Handlers
 **************************************/

/**
   Base template for the class.  If this one gets chosen, then there
   is a mismatch somewhere, so it will just static_assert
 */
template <typename returnT, typename argT, typename classT, typename dataT, auto funcT>
class SSTHandler2 : public SSTHandlerBase<returnT, argT>
{

    // This has to be dependent on a tenplate, otherwise it always
    // assers.  Need to make sure it covers both cases to assert
    // if this template is expanded.
    static_assert(std::is_fundamental<dataT>::value, "Mismatched handler templates.");
    static_assert(!std::is_fundamental<dataT>::value, "Mismatched handler templates.");
};


/**
 * Handler class with user-data argument
 */
template <typename returnT, typename argT, typename classT, typename dataT, returnT (classT::*funcT)(argT, dataT)>
class SSTHandler2<returnT, argT, classT, dataT, funcT> : public SSTHandlerBase<returnT, argT>
// template <typename returnT, typename argT, typename classT, typename dataT, auto funcT>
// class SSTHandler2 : public SSTHandlerBase<returnT, argT>
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
        // SSTHandlerBase<returnT, argT>::serialize_order(ser);
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
        // SSTHandlerBase<returnT, argT>::serialize_order(ser);
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
        // SSTHandlerBase<returnT, argT>::serialize_order(ser);
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
        // SSTHandlerBase<returnT, argT>::serialize_order(ser);
        ser& object;
    }

    ImplementSerializable(SSTHandler2)
};

#if 0

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
        SSTHandlerBaseNoArgs<returnT>(),
        member(member),
        object(object)
    {}

    void operator_impl() override { return (object->*member)(); }

    NotSerializable(SSTHandlerNoArgs)
};
#endif

} // namespace SST

#endif // SST_CORE_SSTHANDLER_H
