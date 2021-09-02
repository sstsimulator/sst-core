// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SSTHANDLER_H
#define SST_CORE_SSTHANDLER_H

namespace SST {

// This file contains base classes for use as various handlers (object
// encapsulating callback functions) in SST.  The classes expect to
// encapsulate a pointer to an object and a pointer to a member
// function of the objects class.  The classes also allow you to
// optionally add one additional piece of static data to be passed
// into the callback function along with any data provided by the
// caller.  There are two versions of this class, one that has no data
// passed from the caller (ending with SSTHandler0), and one that has
// a single item passed from the caller (SSTHandler1).

// These classes provide the full functionality of the handlers and
// can be added to a class with the "using" keyword, as follows (a
// class can use any type name they'd like in place of HandlerBase and
// Handler, though those names are preferred for consistency):

// using HandlerBase = SSTHandlerBase<return_type_of_callback, arg_type_of_callback>;

// template <typename classT, typename dataT = void>
// using Handler = SSTHandler<return_type_of_callback, arg_type_of_callback classT, dataT>;

// Or:

// using HandlerBase = SSTHandlerBaseNoArgs<return_type_of_callback>;

// template <return_type_of_callback, typename classT, typename dataT = void>
// using Handler = SSTHandlerNoArgs<return_type_of_callback, classT, dataT>;

// The handlers are then instanced as follows:

// new Class::Handler<Class>(this, &Class::callback_function)

// Or:

// new Class::Handler<Class,int>(this, &Class::callback_function, 1)


/// Functor classes for Event handling

/// Handler with 1 argument to callback from caller
template <typename returnT, typename argT>
class SSTHandlerBase
{
public:
    /** Handler function */
    virtual returnT operator()(argT) = 0;
    virtual ~SSTHandlerBase() {}
};


/**
 * Event Handler class with user-data argument
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
    SSTHandler(classT* const object, PtrMember member, dataT data) : object(object), member(member), data(data) {}

    returnT operator()(argT arg) { return (object->*member)(arg, data); }
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
    SSTHandler(classT* const object, PtrMember member) : member(member), object(object) {}

    returnT operator()(argT arg) { return (object->*member)(arg); }
};


/// Handler with no arguments to callback from caller
template <typename returnT>
class SSTHandlerBaseNoArgs
{
public:
    /** Handler function */
    virtual returnT operator()() = 0;
    virtual ~SSTHandlerBaseNoArgs() {}
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
    SSTHandlerNoArgs(classT* const object, PtrMember member, dataT data) : object(object), member(member), data(data) {}

    void operator()() { return (object->*member)(data); }
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
    SSTHandlerNoArgs(classT* const object, PtrMember member) : member(member), object(object) {}

    void operator()() { return (object->*member)(); }
};


} // namespace SST

#endif // SST_CORE_SSTHANDLER_H
