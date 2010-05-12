#ifndef _CPU_H
#define _CPU_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <memoryDev.h>
#include <OffCpuIF.h>
#include <McOpteron.h>

#ifndef CPUV2_DBG
#define CPUV2_DBG
#endif

using namespace SST;

class Cpu : public Component, OffCpuIF {

    virtual void  memoryAccess(OffCpuIF::access_mode mode,
                               unsigned long long address,
                               unsigned long data_size);
    virtual void  NICAccess(OffCpuIF::access_mode mode,
                            unsigned long data_size);

    typedef enum { RUN, STOP } inst_t;

    struct Foo {
        inst_t inst;
    };

    public:
        Cpu( ComponentId_t id, Params_t& params );

    private:

        Cpu( const Cpu& c );
        Cpu();
        int Finish();

        bool clock( Cycle_t );
        bool processEvent( Event *e );

    private:
        typedef MemoryDev< uint64_t, unsigned int > memDev_t;

        ClockHandler_t*         m_clockHandler;
        std::string             m_frequency;

        McOpteron *mcCpu;
        unsigned long cyclesAtLastClock;
        const char *instfile;
        const char *configfile;
        const char *appfile;
        const char *distfile;
        const char *outputfile;
        const char *debugfile;
        unsigned long memCookie;

        memDev_t::addr_t   m_pc;
        memDev_t::addr_t   m_pcStop;

        memDev_t*          m_memory; 

        Log< CPUV2_DBG >&       m_dbg;
        Log<>                   m_log;
};

#endif
