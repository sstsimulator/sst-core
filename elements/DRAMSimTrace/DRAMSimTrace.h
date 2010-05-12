#ifndef _DRAMSIMFRONT_H
#define _DRAMSIMFRONT_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <memoryDev.h>
#include <fstream>
#include <Transaction.h>

#ifndef DRAMSIMT_DBG
#define DRAMSIMT_DBG
#endif

using namespace SST;

class DRAMSimTrace : public Component {

    public: // functions

        DRAMSimTrace( ComponentId_t id, Params_t& params );

    private: // functions

        DRAMSimTrace();
        DRAMSimTrace( const DRAMSimTrace& c );
        bool clock( Cycle_t );

    private: // types 

        struct Op {
            uint64_t                        addr;
            enum DRAMSim::TransactionType   type;
        };

        typedef MemoryDev< uint64_t, Op* >  memDev_t;

    private: // data

        Op*                     m_onDeckOp;

        std::ifstream           m_traceFile;
        TraceType               m_traceType;
        memDev_t*               m_memory;

        uint64_t                m_clockCycle;
        Log< DRAMSIMT_DBG >&    m_dbg;
        Log<>                   m_log;
};

#endif
