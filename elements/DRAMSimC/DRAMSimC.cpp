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


#include <DRAMSimC.h>

#define DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

DRAMSimC::DRAMSimC( ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_printStats("no"),
    m_dbg( *new Log< DRAMSIMC_DBG >( "DRAMSimC::", false ) ),
    m_log( *new Log< >( "INFO DRAMSimC: ", false ) )
{
    std::string frequency = "2.2 GHz";
    std::string systemIniFilename = "ini/system.ini";
    std::string deviceIniFilename = "";

    if ( params.find( "info" ) != params.end() ) {
        if ( params[ "info" ].compare( "yes" ) == 0 ) {
            m_log.enable();
        }
    }

    if ( params.find( "debug" ) != params.end() ) {
        if ( params[ "debug" ].compare( "yes" ) == 0 ) {
            m_dbg.enable();
        }
    }

    DBG("new id=%lu\n",id);

    m_memChan = new memChan_t( *this, params, "bus" );

    std::string     pwdString = "";

    Params_t::iterator it = params.begin();
    while( it != params.end() ) {
        DBG("key=%s value=%s\n",
        it->first.c_str(),it->second.c_str());
        if ( ! it->first.compare("deviceini") ) {
            deviceIniFilename = it->second;
        }
        if ( ! it->first.compare("systemini") ) {
            systemIniFilename = it->second;
        }
        if ( ! it->first.compare("pwd") ) {
            pwdString = it->second;
        }
        if ( ! it->first.compare("printStats") ) {
            m_printStats = it->second;
        }
        ++it;
    }

    DBG("pwd %s\n",pwdString.c_str());

    deviceIniFilename = pwdString + "/" + deviceIniFilename;
    systemIniFilename = pwdString + "/" + systemIniFilename;

    m_log.write("device ini %s\n",deviceIniFilename.c_str());
    m_log.write("system ini %s\n",systemIniFilename.c_str());

    ClockHandler_t* handler;
    handler = new EventHandler< DRAMSimC, bool, Cycle_t >
                                        ( this, &DRAMSimC::clock );

    m_log.write("freq %s\n",frequency.c_str());
    TimeConverter* tc = registerClock( frequency, handler );
    m_log.write("period %ld\n",tc->getFactor());


    m_memorySystem = new MemorySystem(0, deviceIniFilename,
                    systemIniFilename, "", "");
    if ( ! m_memorySystem ) {
      _abort(DRAMSimC,"MemorySystem() failed\n");
    }

    Callback< DRAMSimC, void, uint, uint64_t,uint64_t >* readDataCB;
    Callback< DRAMSimC, void, uint, uint64_t,uint64_t >* writeDataCB;

    readDataCB = new Callback< DRAMSimC, void, uint, uint64_t,uint64_t >
      (this, &DRAMSimC::readData);

    writeDataCB = new Callback< DRAMSimC, void, uint, uint64_t,uint64_t >
      (this, &DRAMSimC::writeData);
    m_memorySystem->RegisterCallbacks( readDataCB,writeDataCB, NULL);
}

int DRAMSimC::Finish() 
{
    if ( ! m_printStats.compare("yes" ) ) {
        m_memorySystem->printStats();
    }
    return 0;
}

void DRAMSimC::readData(uint id, uint64_t addr, uint64_t clockcycle)
{
    DBG("id=%d addr=%#lx clock=%lu\n",id,addr,clockcycle);
    memChan_t::event_t* event = new memChan_t::event_t;
    event->addr = addr;
    event->reqType = memChan_t::event_t::READ; 
    event->msgType = memChan_t::event_t::RESPONSE; 
    if ( ! m_memChan->send( event ) ) {
        _abort(DRAMSimC::readData,"m_memChan->send failed\n");
    }
}

void DRAMSimC::writeData(uint id, uint64_t addr, uint64_t clockcycle)
{
    DBG("id=%d addr=%#lx clock=%lu\n",id,addr,clockcycle);
    memChan_t::event_t* event = new memChan_t::event_t;
    event->addr = addr;
    event->reqType = memChan_t::event_t::WRITE; 
    event->msgType = memChan_t::event_t::RESPONSE; 
    if( ! m_memChan->send(event) ) {
        _abort(DRAMSimC::writeData, "m_memChan->send failed\n");
    }
}

bool DRAMSimC::clock( Cycle_t current )
{
    m_memorySystem->update();

    memChan_t::event_t* event;
    while ( m_memChan->recv( &event ) ) 
    {
        DBG("got an event %x\n", event);
        
        TransactionType transType = 
                            convertType( event->reqType );
        DBG("transType=%d addr=%#lx\n", transType, event->addr );
        m_transQ.push_back( Transaction( transType, event->addr, NULL));
    }

    int ret = 1; 
    while ( ! m_transQ.empty() && ret ) {
        if (  ( ret = m_memorySystem->addTransaction( m_transQ.front() ) ) )
        {
	        DBG("addTransaction succeeded %#x\n", m_transQ.front().address);
	        m_transQ.pop_front();
        } else {
	        DBG("addTransaction failed\n");
        }
    }
    return false;
}

extern "C" {
DRAMSimC* DRAMSimCAllocComponent( SST::ComponentId_t id,  
                                    SST::Component::Params_t& params )
{
    return new DRAMSimC( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(DRAMSimC)

BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
                                DRAMSimC, bool, SST::Cycle_t, SST::Time_t )
#endif
