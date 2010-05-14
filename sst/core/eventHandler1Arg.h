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


#ifndef _SST_EVENTHANDLER1ARG_H
#define _SST_EVENTHANDLER1ARG_H

#include <sst/eventFunctor.h>

namespace SST {

template < typename ConsumerT, 
            typename ReturnT,
            typename EventT,
            typename ArgT >
class EventHandler1Arg: public EventHandler< ConsumerT, ReturnT, EventT >
{
    private:
        typedef ReturnT (ConsumerT::*PtrMember)( EventT, ArgT );

    public:
        EventHandler1Arg( ConsumerT* const object,
                            PtrMember member, ArgT arg ) :
            m_object( object ),     
            m_member( member ),
            m_arg( arg )
        { }

        EventHandler1Arg( const EventHandler1Arg< ConsumerT, ReturnT, 
                                        EventT, ArgT >& e ) :
            m_object(e.m_object), 
            m_member(e.m_member),
            m_arg(e.m_arg)
        { }

        virtual ReturnT operator()( EventT event ) {
            return (const_cast<ConsumerT*>(m_object)->*m_member)(event,m_arg);
        }
    private:
        // CONSTRUCT Data
        ConsumerT* const    m_object;
        const PtrMember     m_member;
        ArgT                m_arg;
};

} // namespace SST

#endif
