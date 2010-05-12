#include "cpu.h"

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)


extern "C" {
Cpu* 
mcopteronAllocComponent( SST::ComponentId_t id, 
                         SST::Component::Params_t& params )
{
    fprintf(stderr,"mcopteronAllocComponent\n");
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

    instfile  = "./notavail_opteron-insn.txt";
    configfile  = "./notavail_cpuconfig.ini";
    appfile  = "./notavail_appconfig.ini";
    distfile = "./notavail_usedist.all";
    outputfile = "./mc_output";
    debugfile = 0; // default to stderr

    registerExit();

    m_memory = new memDev_t( *this, params, "MEM" );

    m_clockHandler = new EventHandler< Cpu, bool, Cycle_t >
                                                ( this, &Cpu::clock );

    DBG("MC: get clock var\n");
    if ( params.find("clock") != params.end() ) {
        m_frequency = params["clock"];
    }
    DBG("MC: get mccpu_instfile var\n");
    if ( params.find("mccpu_instfile") != params.end() ) {
        instfile = params["mccpu_instfile"].c_str();
    }
    DBG("MC: get mccpu_outputfile var\n");
    if ( params.find("mccpu_outputfile") != params.end() ) {
        outputfile = params["mccpu_outputfile"].c_str();
    }
    DBG("MC: get mccpu_configfile var\n");
    if ( params.find("mccpu_configfile") != params.end() ) {
        configfile = params["mccpu_configfile"].c_str();
    }
    DBG("MC: get mccpu_appfile var\n");
    if ( params.find("mccpu_appfile") != params.end() ) {
        appfile = params["mccpu_appfile"].c_str();
    }
    DBG("MC: get mccpu_distfile var\n");
    if ( params.find("mccpu_distfile") != params.end() ) {
        distfile = params["mccpu_distfile"].c_str();
    }
    DBG("MC: get mccpu_debugfile var\n");
    if ( params.find("mccpu_debugfile") != params.end() ) {
        debugfile = params["mccpu_debugfile"].c_str();
    }
    DBG("MC: got vars\n");

    m_log.write("-->frequency=%s\n",m_frequency.c_str());

    TimeConverter* tc = registerClock( m_frequency, m_clockHandler );
    if ( ! tc ) {
        _abort(XbarV2,"couldn't register clock handler");
    }

    mcCpu = new McOpteron();
    cyclesAtLastClock = 0;
    DBG(" mc_files: (%s) (%s) (%s) (%s) (%s) (%s)\n", instfile, configfile,
        appfile, distfile, outputfile, debugfile);
    char filename[128];
    if (outputfile) {
       sprintf(filename, "%s.%d", outputfile, (int)Id());
       outputfile = filename; // should delete old one first??
    }
    mcCpu->setOutputFiles(outputfile, debugfile);
    mcCpu->init(instfile, distfile, configfile, appfile, this, 0);
    memCookie = 1000;

    DBG("Done registering clock\n");
}

int Cpu::Finish()
{
    DBG("\n");
    mcCpu->finish(true);
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
       mcCpu->simCycle();
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
