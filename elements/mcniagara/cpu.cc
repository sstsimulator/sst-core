#include <sst_config.h>

#include "cpu.h"

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)


extern "C" {
Cpu* 
mcniagaraAllocComponent( SST::ComponentId_t id, 
                         SST::Component::Params_t& params )
{
    return new Cpu( id, params );
}
}

Cpu::Cpu( ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_frequency( "2.2Ghz" ),
    m_pc( 0x1000 ),
    m_pcStop( m_pc + 0x80 ),
    m_dbg( *new Log< CPUV2_DBG >( "Cpu::") )
{
    DBG( "new id=%lu\n", id );

    inputfile  = "./notavail_insthist.dat";
    iprobfile  = "./notavail_instprob.dat";
    perffile  = "./notavail_perfcnt.dat";
    outputfile = "./mc_output";

    registerExit();

    m_memory = new memDev_t( *this, params, "MEM" );

    m_clockHandler = new EventHandler< Cpu, bool, Cycle_t >
                                                ( this, &Cpu::clock );

    if ( params.find("clock") != params.end() ) {
        m_frequency = params["clock"];
    }
    if ( params.find("mccpu_ihistfile") != params.end() ) {
        inputfile = params["mccpu_ihistfile"].c_str();
    }
    if ( params.find("mccpu_outputfile") != params.end() ) {
        outputfile = params["mccpu_outputfile"].c_str();
    }
    if ( params.find("mccpu_iprobfile") != params.end() ) {
        iprobfile = params["mccpu_iprobfile"].c_str();
    }
    if ( params.find("mccpu_perffile") != params.end() ) {
        perffile = params["mccpu_perffile"].c_str();
    }

    m_log.write("-->frequency=%s\n",m_frequency.c_str());

    TimeConverter* tc = registerClock( m_frequency, m_clockHandler );
    if ( ! tc ) {
        _abort(XbarV2,"couldn't register clock handler");
    }

    mcCpu = new McNiagara();
    cyclesAtLastClock = 0;
    DBG(" mc_files: (%s) (%s) (%s) (%s)\n", inputfile, iprobfile,
        perffile, outputfile);
    mcCpu->init(inputfile, this, iprobfile, perffile, 0, 0);
    memCookie = 1000;

    DBG("Done registering clock\n");
}

int Cpu::Finish()
{
    char filename[128];
    DBG("\n");
    sprintf(filename, "%s.%d", outputfile, (int)Id());
    mcCpu->fini(filename);
    return 0;
}

bool Cpu::clock( Cycle_t current )
{
    unsigned int i, recvCookie;

    while ( ( m_memory->popCookie(recvCookie) ) ) {
       // nothing to do
    }
    DBG("id=%lu currentCycle=%lu inst=%d \n", Id(), current, memCookie );
    m_pc += 8;

    for (i = cyclesAtLastClock; i < getCurrentSimTime(); i++)
       mcCpu->sim_cycle(i);
    cyclesAtLastClock = getCurrentSimTime();

    return false;
}

void Cpu::memoryAccess(OffCpuIF::access_mode mode,
                       long long unsigned int address,
                       long unsigned int data_size)
{
    DBG( "memoryAccess\n");
    if (mode == AM_READ)
       m_memory->read(address, memCookie++);
    else
       m_memory->write(address, memCookie++);
}

void Cpu::NICAccess(OffCpuIF::access_mode mode, long unsigned int data_size)
{
    DBG( "nicAccess\n");
}
