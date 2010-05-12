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


#ifndef _CPU_H
#define _CPU_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <memoryDev.h>

#ifndef CPUV2_DBG
#define CPUV2_DBG
#endif

using namespace SST;

class Cpu : public Component {

    typedef enum { RUN, STOP } inst_t;

    struct Foo {
        inst_t inst;
    };

    public:
        Cpu( ComponentId_t id, Params_t& params );

    private:

        Cpu( const Cpu& c );
        Cpu();

        bool clock( Cycle_t );
        bool processEvent( Event *e );

    private:
        ClockHandler_t*    m_clockHandler;
        std::string        m_frequency;

        typedef MemoryDev< uint64_t, Foo* > memDev_t;

        memDev_t::addr_t   m_pc;
        memDev_t::addr_t   m_pcStop;

        memDev_t*          m_memory; 

        Log< CPUV2_DBG >&  m_dbg;
        Log<>              m_log;
};

#endif
