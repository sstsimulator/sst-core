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


#ifndef SST_CORE_ONESHOT_H
#define SST_CORE_ONESHOT_H

#include <sst/core/sst_types.h>

#include <cinttypes>

#include <sst/core/action.h>

#define _ONESHOT_DBG(fmt, args...)__DBG(DBG_ONESHOT, OneShot, fmt, ## args)

namespace SST {

class TimeConverter;

/**
 * A OneShot Event class.
 *
 * Calls callback functions (handlers) on a specified period
 */
class OneShot : public Action
{
public:
    /////////////////////////////////////////////////

    /** Functor classes for OneShot handling */
    class HandlerBase {
    public:
        /** Function called when Handler is invoked */
        virtual void operator()() = 0;
        virtual ~HandlerBase() {}
    };

    /////////////////////////////////////////////////

    /** Event Handler class with user-data argument
     * @tparam classT Type of the Object
     * @tparam argT Type of the argument
     */
    template <typename classT, typename argT = void>
    class Handler : public HandlerBase {
    private:
        typedef void (classT::*PtrMember)(argT);
        classT*         object;
        const PtrMember member;
        argT            data;
	
    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         * @param data - Additional argument to pass to handler
         */
        Handler(classT* const object, PtrMember member, argT data) :
            object(object),
            member(member),
            data(data)
        {}

        /** Operator to callback OneShot Handler; called 
         *  by the OneShot object to execute the users callback.
         * @param data - The data passed in when the handler was created
         */
        void operator()() override {
            (object->*member)(data);
        }
    };

    /////////////////////////////////////////////////
    
    /** Event Handler class without user-data
     * @tparam classT Type of the Object
     */
    template <typename classT>
    class Handler<classT, void> : public HandlerBase {
    private:
        typedef void (classT::*PtrMember)();
        classT*         object;
        const PtrMember member;

    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         */
        Handler(classT* const object, PtrMember member) :
            object(object),
            member(member)
        {}

        /** Operator to callback OneShot Handler; called 
         *  by the OneShot object to execute the users callback.
         */
        void operator()() override {
            (object->*member)();
        }
    };

    /////////////////////////////////////////////////

    /** Create a new One Shot for a specified time that will callback the 
        handler function.
        Note: OneShot cannot be canceled, and will always callback after
              the timedelay.  
    */
    OneShot(TimeConverter* timeDelay, int priority = ONESHOTPRIORITY);
    ~OneShot();
    
    /** Is OneShot scheduled */
    bool isScheduled() {return m_scheduled;}
    
    /** Add a handler to be called on this OneShot Event */
    void registerHandler(OneShot::HandlerBase* handler);

    /** Print details about the OneShot */
    void print(const std::string& header, Output &out) const override;
    
private:
    typedef std::vector<OneShot::HandlerBase*>  HandlerList_t;
    typedef std::map<SimTime_t, HandlerList_t*> HandlerVectorMap_t;
    
    // Generic constructor for serialization
    OneShot() { }

    // Called by the Simulation (Activity Queue) when delay time as elapsed
    void execute(void) override;

    // Activates this OneShot object, by inserting into the simulation's
    // timeVortex for future execution.
    SimTime_t scheduleOneShot();

    TimeConverter*      m_timeDelay;
    HandlerVectorMap_t  m_HandlerVectorMap;
    bool                m_scheduled;
    
};

} // namespace SST


#endif // SST_CORE_ONESHOT_H
