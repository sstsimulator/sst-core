#include <DRAMSimTrace.h>
//#include <sst/memEvent.h>

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

DRAMSimTrace::DRAMSimTrace( ComponentId_t id, 
                                        Params_t& params ) :
    Component( id ),
    m_clockCycle( 1 ),
    m_onDeckOp( NULL ),
    m_dbg( *new Log< DRAMSIMT_DBG >( "DRAMSimTrace::") )
{
    DBG("new id=%lu\n",id);
    std::string frequency = "2.2Ghz";
    std::string traceFileName = "traces/trace.trc";

    DBG("\n");
    m_memory = new memDev_t( *this, params, "bus" );
    DBG("\n");

    registerExit();

    ClockHandler_t* handler;
    handler = new EventHandler< DRAMSimTrace, bool, Cycle_t >
                                            ( this, &DRAMSimTrace::clock );

    m_log.write(" DRAMSimC freq : %s\n", frequency.c_str() );
    TimeConverter* tc = registerClock( frequency, handler );
    m_log.write(" DRAMSimC  period: %ld\n",tc->getFactor());

    std::string     pwdString = "";

    Params_t::iterator it = params.begin();
    while( it != params.end() ) {
        DBG("key=%s value=%s\n", it->first.c_str(),it->second.c_str());
        if ( ! it->first.compare("tracefile") ) {
            traceFileName = it->second;
        }
        if ( ! it->first.compare("pwd") ) {
            pwdString = it->second;
        }

        ++it;
    }
    traceFileName = pwdString + "/" + traceFileName;

    DBG( "traceFile=%s\n", traceFileName.c_str() );

    std::string temp = traceFileName.substr(traceFileName.find_last_of("/")+1);
    temp = temp.substr(0,temp.find_first_of("_"));

    if(temp=="mase") {
        m_traceType = mase;
    } else if(temp=="k6") {
        m_traceType = k6;
    } else if(temp=="misc") {
        m_traceType = misc;
    } else {
        _abort(DRAMSimTrace,"== Unknown Tracefile Type : %s\n",temp.c_str());
    }

    m_traceFile.open( traceFileName.c_str() );
    if ( ! m_traceFile.is_open() ) {
        _abort(DRAMSimTrace,"couldn't open trace file %s\n",
                    traceFileName.c_str());
    }
}

bool DRAMSimTrace::clock( Cycle_t current )
{
//    DBG( "id=%lu currentCycle=%lu\n", Id(), current );

    extern void *parseTraceFileLine(string &line, uint64_t &addr,
        enum DRAMSim::TransactionType &transType, uint64_t &clockCycle, 
                                TraceType type);

    while ( current == m_clockCycle ) {
        if ( ! m_onDeckOp ) {

            if ( m_traceFile.eof() )
            {
                DBG("\n");
                return true;
            }

            string line;
            getline( m_traceFile, line );
            if ( line.size() == 0 ) 
            {
                DBG("\n");
                return true;
            }

            m_onDeckOp = new Op;
	        uint64_t tempAddr(0);
            enum DRAMSim::TransactionType transType;

            // running with -DNO_STORAGE, ignore data coming from this function
            parseTraceFileLine( line, tempAddr, transType, 
                                            m_clockCycle, m_traceType );
            m_onDeckOp->type = transType;
	        m_onDeckOp->addr = (unsigned long)tempAddr;
            DBG("%lu: read, next %lu\n", current, m_clockCycle);
        }

        if ( current == m_clockCycle ) {
            DBG("%lu: sending addr=%#lx type=%d m_clockCycle=%lu\n",
               current, m_onDeckOp->addr, m_onDeckOp->type, m_clockCycle );
            
            switch ( m_onDeckOp->type ) 
            {
                case DRAMSim::DATA_READ:
                    m_memory->read( m_onDeckOp->addr, m_onDeckOp  );    
                    break;
                case DRAMSim::DATA_WRITE:
                    m_memory->write( (unsigned long) m_onDeckOp->addr );    
                    delete m_onDeckOp;
                    break;
            }
            m_onDeckOp = NULL;
        }
    }

    Op* cookie;
    if ( m_memory->popCookie( cookie ) ) {
        DBG("got cookie addr=%#lx\n", cookie->addr );
        delete cookie;
    }
 
    return false;
}

extern "C" {
DRAMSimTrace* DRAMSimTraceAllocComponent( SST::ComponentId_t id,
					  SST::Component::Params_t& params )
{
    return new DRAMSimTrace( id, params );
}
}
