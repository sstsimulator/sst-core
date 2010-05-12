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


#ifndef _DRAMSIMC_H
#define _DRAMSIMC_H

#include <sst/log.h>
#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <memoryChannel.h>
#include <MemorySystem.h>

using namespace std;
using namespace SST;

#ifndef DRAMSIMC_DBG
#define DRAMSIMC_DBG 0
#endif

class DRAMSimC : public Component {

    public: // functions

        DRAMSimC( ComponentId_t id, Params_t& params );
        int Finish();

    private: // types

        typedef MemoryChannel<uint64_t> memChan_t;

    private: // functions

        DRAMSimC( const DRAMSimC& c );
        bool clock( Cycle_t  );

        inline DRAMSim::TransactionType 
                        convertType( memChan_t::event_t::reqType_t type );

        void readData(uint id, uint64_t addr, uint64_t clockcycle);
        void writeData(uint id, uint64_t addr, uint64_t clockcycle);

        std::deque<Transaction> m_transQ;
        MemorySystem*           m_memorySystem;
        memChan_t*              m_memChan;
        std::string             m_printStats;
        Log< DRAMSIMC_DBG >&    m_dbg;
        Log<>&                  m_log;

#if WANT_CHECKPOINT_SUPPORT
        BOOST_SERIALIZE {
            _AR_DBG(DRAMSimC,"start\n");
            BOOST_VOID_CAST_REGISTER( DRAMSimC*, Component* );
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Component );
            ar & BOOST_SERIALIZATION_NVP( bus );
            _AR_DBG(DRAMSimC,"done\n");
        }
        SAVE_CONSTRUCT_DATA( DRAMSimC ) {
            _AR_DBG(DRAMSimC,"\n");
            ComponentId_t     id     = t->_id;
            Clock*            clock  = t->_clock;
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP( id );
            ar << BOOST_SERIALIZATION_NVP( clock );
            ar << BOOST_SERIALIZATION_NVP( params );
        } 
        LOAD_CONSTRUCT_DATA( DRAMSimC ) {
            _AR_DBG(DRAMSimC,"\n");
            ComponentId_t     id;
            Clock*            clock;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP( id );
            ar >> BOOST_SERIALIZATION_NVP( clock );
            ar >> BOOST_SERIALIZATION_NVP( params );
            ::new(t)DRAMSimC( id, clock, params );
        } 
#endif
};

inline DRAMSim::TransactionType 
            DRAMSimC::convertType( memChan_t::event_t::reqType_t type )
{
    switch( type ) {
        case memChan_t::event_t::READ:
            return DRAMSim::DATA_READ;
        case memChan_t::event_t::WRITE:
            return DRAMSim::DATA_WRITE;
    }
    return (DRAMSim::TransactionType)-1;
}

#endif
