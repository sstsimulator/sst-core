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


#include "sst_config.h"
#include "sst/core/serialization/core.h"
#include "sst/core/serialization/types.h"

#include "sst/core/exit.h"
#include "sst/core/exitEvent.h"
#include "sst/core/simulation.h"

namespace SST {

Exit::Exit( Simulation* sim, TimeConverter* period ) :
    m_functor( new EventHandler<Exit,bool,Event*> (this,&Exit::handler ) ),
    m_refCount( 0 ),
    m_period( period ) 
{
    _EXIT_DBG("\n");
    ExitEvent* event = new ExitEvent();

    sim->insertEvent( period->getFactor(), event, m_functor );
}

bool Exit::refInc( ComponentId_t id )
{
    _EXIT_DBG( "refCount=%d\n", m_refCount );

    if ( m_idSet.find( id ) != m_idSet.end() ) {
        _DBG( Exit, "component multiple increment\n" );
        return true;
    } 

    m_idSet.insert( id );

    ++m_refCount;

    return false;
}

bool Exit::refDec( ComponentId_t id )
{
    _EXIT_DBG("refCount=%d\n",m_refCount );

    if ( m_idSet.find( id ) == m_idSet.end() ) {
        _DBG( Exit, "component multiple decrement\n" );
        return true;
    } 


    if ( m_refCount == 0 ) {
        _abort( Exit, "refCount is already 0\n" );
        return true;
    }

    m_idSet.erase( id );

    --m_refCount;

    return false;
}

bool Exit::handler( Event* e )
{
    Simulation *sim = Simulation::getSimulation();

    _EXIT_DBG("%lu\n", (unsigned long) sim->getCurrentSimCycle());
    boost::mpi::communicator world; 

    int value = ( m_refCount > 0 );
    int out;

    all_reduce( world, &value, 1, &out, std::plus<int>() );  

    _EXIT_DBG("%d\n",out);

    if ( out ) {
        SimTime_t next = sim->getCurrentSimCycle() + 
            m_period->getFactor();
        sim->insertEvent( next, e, m_functor );
    }
    return ( out == 0 ); 
}

} // namespace SST
