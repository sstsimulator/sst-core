// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CLOCK_H
#define SST_CLOCK_H

#include <deque>

#include <sst/core/action.h>
//#include <sst/core/clockHandler.h>

#define _CLE_DBG( fmt, args...)__DBG( DBG_CLOCK, Clock, fmt, ## args )

namespace SST {

class TimeConverter;

class Clock : public Action
{
public:
    typedef enum { DEFAULT, PRE, POST } Which_t;

    Clock( TimeConverter* period );

    // Functor classes for Clock handling
    class HandlerBase {
    public:
	virtual bool operator()(Cycle_t) = 0;
	virtual ~HandlerBase() {}
    };
    

    template <typename classT, typename argT = void>
    class Handler : public HandlerBase {
    private:
	typedef bool (classT::*PtrMember)(Cycle_t, argT);
	classT* object;
	const PtrMember member;
	argT data;
	
    public:
	Handler( classT* const object, PtrMember member, argT data ) :
	    object(object),
	    member(member),
	    data(data)
	{}

	    bool operator()(Cycle_t cycle) {
		return (object->*member)(cycle,data);
	    }
    };
    
    template <typename classT>
    class Handler<classT, void> : public HandlerBase {
    private:
	typedef bool (classT::*PtrMember)(Cycle_t);
	classT* object;
	const PtrMember member;
	
    public:
	Handler( classT* const object, PtrMember member ) :
	    object(object),
	    member(member)
	{}

	    bool operator()(Cycle_t cycle) {
		return (object->*member)(cycle);
	    }
    };


    
//     bool HandlerRegister( Which_t which, ClockHandler_t* handler ); 
//     bool HandlerUnregister( Which_t which, ClockHandler_t* handler, bool& empty );
    bool HandlerRegister( Which_t which, Clock::HandlerBase* handler ); 
    bool HandlerUnregister( Which_t which, Clock::HandlerBase* handler, bool& empty );

    
private:
    typedef std::deque<Clock::HandlerBase*> HandlerMap_t;

    Clock() { }

    void execute( void );

    Cycle_t         currentCycle;
    TimeConverter*  period;
    HandlerMap_t    handlerMap[3];

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
        ar & BOOST_SERIALIZATION_NVP(currentCycle);
        ar & BOOST_SERIALIZATION_NVP(period);
    }
};

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Clock)

#endif // SST_CLOCK_H
