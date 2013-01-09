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

Exit::~Exit()
{
    m_idSet.clear();
}
    
bool Exit::refInc( ComponentId_t id )
{
    _EXIT_DBG( "refCount=%d\n", m_refCount );

    
    if ( m_idSet.find( id ) != m_idSet.end() ) {
        _DBG( Exit, "component (%s) multiple increment\n",
                Simulation::getSimulation()->getComponent(id)->getName().c_str() );
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
        _DBG( Exit, "component (%s) multiple decrement\n",
                Simulation::getSimulation()->getComponent(id)->getName().c_str() );
        return true;
    } 


    if ( m_refCount == 0 ) {
        _abort( Exit, "refCount is already 0\n" );
        return true;
    }

    m_idSet.erase( id );

    --m_refCount;

    if ( single_rank && m_refCount == 0 ) {
	Simulation* sim = Simulation::getSimulation();
	// sim->insertActivity( sim->getCurrentSimCycle() + m_period->getFactor(), this );
	sim->insertActivity( sim->getCurrentSimCycle() + 1, this );
    }

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

    // If out is 0, then it's time to end
    if ( !out ) {
	endSimulation();
    }
    // Reinsert into TimeVortex.  We do this even when ending so that
    // it will get deleted with the TimeVortex on termination.  We do
    // this in case we exit with a StopEvent instead.  In that case,
    // there is no way to know if the Exit object is deleted in the
    // TimeVortex or not, so we just make sure it is always deleted
    // there.
    SimTime_t next = sim->getCurrentSimCycle() + 
	m_period->getFactor();
    sim->insertActivity( next, this );
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
