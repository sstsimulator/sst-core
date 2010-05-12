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


#include <sst_config.h>
#include "cpu_PowerAndData.h"

#include <sst/memEvent.h>


bool Cpu_PowerAndData::clock( Cycle_t current)
{
    //_CPU_POWERANDDATA_DBG("id=%lu currentCycle=%lu \n", Id(), current );

    MemEvent* event = NULL; 
    mycore_temperature = 360;
   

    if ( state == SEND ) { 
        if ( ! event ) event = new MemEvent();

        if ( who == WHO_MEM ) { 
            event->address = 0x1000; 
            who = WHO_NIC;
        } else {
            event->address = 0x10000000; 
            who = WHO_MEM;
        }

        _CPU_POWERANDDATA_DBG("send a MEM event address=%#lx\n", event->address );
	
	

        mem->Send( (Cycle_t)3, event );
        state = WAIT;

	//p_int = getMonitorIntData("CPUcounts");
	//if (p_int.first)
	//    std::cout << "ID " << Id() << ": CPUcounts = " << *(p_int.second) << std::endl;

    } else {
        if ( ( event = static_cast< MemEvent* >( mem->Recv() ) ) ) {
            _CPU_POWERANDDATA_DBG("got a MEM event address=%#lx\n", event->address );


            state = SEND;
	    if (Id() == 1){
	       counts++;
	       num_il1_read++;
	       num_branch_read = num_branch_read + 2;
	       num_RAS_read = num_RAS_read + 2;
	    }
	    else{
		counts = counts+2;
		num_il1_read = num_il1_read +2;
	    }
        }
    }
    return false;
}

bool Cpu_PowerAndData::pushData( Cycle_t current)
{
        //_CPU_POWERANDDATA_DBG("id=%lu currentCycle=%lu \n", Id(), current );
    		

	//printf("id = %lu: currentCoreTime = %lu and currentCompCycle = %lu\n", Id(), getCurrentCoreTime(current), current);
	//if(current % pushFreq == 0){	  
	  if(isTimeToPush(current, pushIntrospector.c_str())){
	    
	    // set up counts 
	    //first reset all counts to zero
	    power->resetCounts(mycounts); 
	    //then set up "this"-related counts
	    mycounts.branch_read=2;  mycounts.branch_write=2; mycounts.RAS_read=2; mycounts.RAS_write=2;
	    mycounts.il1_read=1; mycounts.il1_readmiss=0; mycounts.IB_read=2; mycounts.IB_write=2; mycounts.BTB_read=2; mycounts.BTB_write=2;
	    mycounts.int_win_read=4; mycounts.int_win_write=2; mycounts.fp_win_read=4; mycounts.fp_win_write=2; mycounts.ROB_read=2; mycounts.ROB_write=2;
	    mycounts.iFRAT_read=2; mycounts.iFRAT_write=2; mycounts.iFRAT_search=0; mycounts.fFRAT_read=2; mycounts.fFRAT_write=2; mycounts.fFRAT_search=0; mycounts.iRRAT_write=2;
            mycounts.fRRAT_write=2; mycounts.ifreeL_read=2; mycounts.ifreeL_write=4; mycounts.ffreeL_read=2; mycounts.ffreeL_write=4; mycounts.idcl_read=0; mycounts.fdcl_read=0;
	    mycounts.dl1_read=1; mycounts.dl1_readmiss=0; mycounts.dl1_write=1; mycounts.dl1_writemiss=0; mycounts.LSQ_read=1; mycounts.LSQ_write=1;
	    mycounts.itlb_read=1; mycounts.itlb_readmiss=0; mycounts.dtlb_read=1; mycounts.dtlb_readmiss=0;
	    mycounts.int_regfile_reads=2; mycounts.int_regfile_writes=2; mycounts.float_regfile_reads=2; mycounts.float_regfile_writes=2; mycounts.RFWIN_read=2; mycounts.RFWIN_write=2;
	    mycounts.bypass_access=1;
	    mycounts.router_access=1;
	    mycounts.L2_read=1; mycounts.L2_readmiss=0; mycounts.L2_write=1; mycounts.L2_writemiss=0; 
	    mycounts.L3_read=1; mycounts.L3_readmiss=0; mycounts.L3_write=1; mycounts.L3_writemiss=0;
	    mycounts.L1Dir_read=1; mycounts.L1Dir_readmiss=0; mycounts.L1Dir_write=1; mycounts.L1Dir_writemiss=0; mycounts.L2Dir_read=1; mycounts.L2Dir_readmiss=0; mycounts.L2Dir_write=1;
            mycounts.L2Dir_writemiss=0;
	    mycounts.memctrl_read=1; mycounts.memctrl_write=1;
	

	    // McPAT beta 
	    pdata = power->getPower(current, CACHE_IL1, mycounts, 1);
	    regPowerStats(pdata);
	    pstats = readPowerStats(this);
	    using namespace io_interval; std::cout <<"ID " << Id() <<": current total power = " << pstats.currentPower << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": leakage power = " << pstats.leakagePower << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": runtime power = " << pstats.runtimeDynamicPower << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": TDP = " << pstats.TDP << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": total energy = " << pstats.totalEnergy << " J" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": peak power = " << pstats.peak << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": current cycle = " << pstats.currentCycle << std::endl;
	}
	return false;
}








extern "C" {
Cpu_PowerAndData* cpu_PowerAndDataAllocComponent( SST::ComponentId_t id,
                                    SST::Component::Params_t& params )
{
    return new Cpu_PowerAndData( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT2
BOOST_CLASS_EXPORT(Cpu_PowerAndData)

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Cpu_PowerAndData, bool, SST::Cycle_t, SST::Time_t )
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Cpu_PowerAndData, bool, SST::Cycle_t)
#endif

