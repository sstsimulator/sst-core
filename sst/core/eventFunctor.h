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



#ifndef _SST_EVENTFUNCTOR_H
#define _SST_EVENTFUNCTOR_H

#include <sst/core/boost.h>
#include <sst/core/debug.h>
#include <sst/core/sst.h>

namespace SST {

    // This is the old version for 2 parameters.  Keep it around, just in case.
#if 0
template <typename ReturnT, typename Param1T, typename Param2T>
class EventHandlerBase
{
    public:
        BOOST_SERIALIZE {
            _AR_DBG(EventHandlerBase,"\n");
        }
        virtual ReturnT operator()(Param1T, Param2T) = 0;
        virtual ~EventHandlerBase() {
	}
};

template <typename ConsumerT, typename ReturnT, 
                        typename Param1T, typename Param2T >
class EventHandler: public EventHandlerBase<ReturnT,Param1T,Param2T>
{
    private:
        typedef ReturnT (ConsumerT::*PtrMember)(Param1T,Param2T);

    public:
        EventHandler( ConsumerT* const object, PtrMember member) :
                object(object), member(member) {
        }

        EventHandler( const EventHandler<ConsumerT,ReturnT,Param1T,Param2T>& e ) :
                object(e.object), member(e.member) {
        }

        ReturnT operator()(Param1T param1, Param2T param2) {
            return (const_cast<ConsumerT*>(object)->*member)(param1,param2);
        }

    private:

        // CONSTRUCT Data
        ConsumerT* const object;
        const PtrMember  member;

        BOOST_SERIALIZE {
            _AR_DBG(EventHandler,"start\n");
            typedef EventHandler<ConsumerT,ReturnT,Param1T,Param2T> tmp1;
            typedef EventHandlerBase<ReturnT,Param1T,Param2T> tmp2;
            BOOST_VOID_CAST_REGISTER(tmp1*,tmp2*);
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(tmp2);
            _AR_DBG(EventHandler,"done\n");
        }

        typedef struct {
		    uint64_t data[2];
	    } member_t;

        template<class Archive> 
        friend void save_construct_data 
            (Archive & ar, const EventHandler<ConsumerT,ReturnT,Param1T,Param2T> * t,
                    const unsigned int file_version) 
        {
            ConsumerT* object = t->object;
            PtrMember tmp        = t->member;

            member_t *member_ptr = reinterpret_cast<member_t*>(&tmp);
            uint64_t member0  = member_ptr->data[0];
            uint64_t member1  = member_ptr->data[1];

            _AR_DBG(EventHandler,"object=%p member=%#lx %#lx\n",
											object,member1,member0);

            ar << BOOST_SERIALIZATION_NVP( object );
            ar << BOOST_SERIALIZATION_NVP( member0 );
            ar << BOOST_SERIALIZATION_NVP( member1 );
        }

        template<class Archive> 
        friend void load_construct_data 
            (Archive & ar, EventHandler<ConsumerT,ReturnT,Param1T,Param2T> * t, 
                    const unsigned int file_version)
        {
            _AR_DBG(EventHandler,"\n");
            ConsumerT* object;
            member_t   member;
            uint64_t   member0; 
            uint64_t   member1;

            ar >> BOOST_SERIALIZATION_NVP( object );
            ar >> BOOST_SERIALIZATION_NVP( member0 );
            ar >> BOOST_SERIALIZATION_NVP( member1 );

            member.data[0] = member0;
            member.data[1] = member1;

            _AR_DBG(EventHandler,"object=%p member=%#lx %#lx\n",
											object,member1,member0);

            PtrMember* tmp_ptr;
            tmp_ptr = reinterpret_cast<PtrMember*>(&member);

            ::new(t)EventHandler<ConsumerT,ReturnT,Param1T,Param2T>
													(object,*tmp_ptr );
        }
};
#endif

  /** Pure virtual base class for event handlers */
template <typename ReturnT, typename Param1T>
class EventHandlerBase
{
    public:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            _AR_DBG(EventHandlerBase,"\n");
        }

        virtual ReturnT operator()(Param1T) = 0;
        virtual ~EventHandlerBase() {
	}
};

  /** Simple event handler functor. 
      
      Creates an event handler which, when triggered, calls a member
      function of a target Component with the event as argument. I.e.

      component->member_function(event);

      @tparam ConsumerT Computer type which recieves the event
      @tparam ReturnT return type of the event
      @tparam Param1T Parameter which the event handler recieves
  */ 
template <typename ConsumerT, typename ReturnT,typename Param1T>
class EventHandler: public EventHandlerBase<ReturnT,Param1T>
{
    private:
        typedef ReturnT (ConsumerT::*PtrMember)(Param1T);

    public:
  /** Constructor 

      @param object Object (usually a Component) upon which the member
      function is invoked

      @param member member function to invoke */
        EventHandler( ConsumerT* const object, PtrMember member) :
                object(object), member(member) {
        }

  /** Copy Constructor */
        EventHandler( const EventHandler<ConsumerT,ReturnT,Param1T>& e ) :
                object(e.object), member(e.member) {
        }

  /** Operator called when event is triggered by the simulator core.
      Calls the Component's member function with the event as
      argument */ 
        virtual ReturnT operator()(Param1T param1) {
            return (const_cast<ConsumerT*>(object)->*member)(param1);
        }

    protected:
        EventHandler() :
            object(NULL), member(NULL)
        { }

    private:

        // CONSTRUCT Data
        ConsumerT* const object;
        const PtrMember  member;

	//#if WANT_CHECKPOINT_SUPPORT

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            _AR_DBG(EventHandler,"start\n");
            typedef EventHandler<ConsumerT,ReturnT,Param1T> tmp1;
            typedef EventHandlerBase<ReturnT,Param1T> tmp2;
            boost::serialization::
                void_cast_register(static_cast<tmp1*>(NULL), 
                                   static_cast<tmp2*>(NULL));
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(tmp2);
            _AR_DBG(EventHandler,"done\n");
        }

        typedef struct {
		    uint64_t data[2];
	    } member_t;

        template<class Archive> 
        friend void save_construct_data 
            (Archive & ar, const EventHandler<ConsumerT,ReturnT,Param1T> * t,
                    const unsigned int file_version) 
        {
            ConsumerT* object = t->object;
            PtrMember tmp        = t->member;

            member_t *member_ptr = reinterpret_cast<member_t*>(&tmp);
            uint64_t member0  = member_ptr->data[0];
            uint64_t member1  = member_ptr->data[1];

            _AR_DBG(EventHandler,"object=%p member=%#lx %#lx\n",
											object,
                                            (unsigned long) member1,
                                            (unsigned long) member0);

            ar << BOOST_SERIALIZATION_NVP( object );
            ar << BOOST_SERIALIZATION_NVP( member0 );
            ar << BOOST_SERIALIZATION_NVP( member1 );
        }

        template<class Archive> 
        friend void load_construct_data 
            (Archive & ar, EventHandler<ConsumerT,ReturnT,Param1T> * t, 
                    const unsigned int file_version)
        {
            _AR_DBG(EventHandler,"\n");
            ConsumerT* object;
            member_t   member;
            uint64_t   member0; 
            uint64_t   member1;

            ar >> BOOST_SERIALIZATION_NVP( object );
            ar >> BOOST_SERIALIZATION_NVP( member0 );
            ar >> BOOST_SERIALIZATION_NVP( member1 );

            member.data[0] = member0;
            member.data[1] = member1;

            _AR_DBG(EventHandler,"object=%p member=%#lx %#lx\n",
											object,
                                            (unsigned long) member1,
                                            (unsigned long) member0);

            PtrMember* tmp_ptr;
            tmp_ptr = reinterpret_cast<PtrMember*>(&member);

            ::new(t)EventHandler<ConsumerT,ReturnT,Param1T>
													(object,*tmp_ptr );
        }

	//#endif
	
};
} // namespace SST

#endif
