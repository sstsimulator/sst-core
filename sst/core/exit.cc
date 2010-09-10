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

#include "sst/core/exit.h"
#include "sst/core/simulation.h"

namespace SST {

Exit::Exit( Simulation* sim, TimeConverter* period, bool single_rank ) :
    Action(),
//     m_functor( new EventHandler<Exit,bool,Event*> (this,&Exit::handler ) ),
    m_refCount( 0 ),
    m_period( period ),
    single_rank(single_rank)	
{
    _EXIT_DBG("\n");
    
    setPriority(99);
    if (!single_rank) sim->insertActivity( period->getFactor(), this );
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

    if ( single_rank && m_refCount == 0 ) Simulation::getSimulation()->insertActivity( m_period->getFactor(), this );

    return false;
}

// bool Exit::handler( Event* e )
void Exit::execute( void )
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
        sim->insertActivity( next, this );
    }
    else {
	endSimulation();
    }
//     return ( out == 0 ); 
}


template<class Archive>
void
Exit::serialize(Archive & ar, const unsigned int version)
{
    printf("begin Exit::serialize\n");
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
    ar & BOOST_SERIALIZATION_NVP(m_refCount);
    ar & BOOST_SERIALIZATION_NVP(m_period);
    ar & BOOST_SERIALIZATION_NVP(m_idSet);
    ar & BOOST_SERIALIZATION_NVP(single_rank);
    printf("end Exit::serialize\n");
}

} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Exit::serialize)
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Exit);
