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


#include <sst_config.h>

#include "cpu.h"

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

Cpu::Cpu( ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_frequency( "2.2Ghz" ),
    m_pc( 0x1000 ),
    m_pcStop( m_pc + 0x80 ),
    m_dbg( *new Log< CPUV2_DBG >( "Cpu::") )
{
    DBG( "new id=%lu\n", id );

    registerExit();

    m_memory = new memDev_t( *this, params, "MEM" );

    m_clockHandler = new EventHandler< Cpu, bool, Cycle_t >
                                                ( this, &Cpu::clock );

    if ( params.find("clock") != params.end() ) {
        m_frequency = params["clock"];
    }

    m_log.write("-->frequency=%s\n",m_frequency.c_str());

    TimeConverter* tc = registerClock( m_frequency, m_clockHandler );
    if ( ! tc ) {
        _abort(XbarV2,"couldn't register clock handler");
    }

    DBG("Done registering clock\n");
}

bool Cpu::clock( Cycle_t current )
{
    Foo* foo;

    while ( m_memory->popCookie( foo ) ) {
    
        if ( foo ) {
            if ( foo->inst == STOP ) {
                DBG("unregister\n" );
                unregisterExit();
                return false;
            } 
            delete foo;
        }
    }

    if ( m_pc > m_pcStop ) {
        return false;
    }

    foo = new Foo;

    foo->inst = RUN;

    if ( m_pc == m_pcStop ) {
        foo->inst = STOP;
    }

    DBG("id=%lu currentCycle=%lu inst=%d \n", Id(), current, foo->inst );

    if ( ! m_memory->read( m_pc, foo ) ) {
        DBG("id=%lu currentCycle=%lu failed\n", Id(), current);
        return false;
    }
    
    m_pc += 8;

    return false;
}
