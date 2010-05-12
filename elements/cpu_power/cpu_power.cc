#include <sst_config.h>
#include "cpu_power.h"

#include <sst/memEvent.h>


bool Cpu_power::clock( Cycle_t current)
{
    //_CPU_POWER_DBG("id=%lu currentCycle=%lu \n", Id(), current );

    MemEvent* event = NULL; 

    if ( state == SEND ) { 
        if ( ! event ) event = new MemEvent();

        if ( who == WHO_MEM ) { 
            event->address = 0x1000; 
            who = WHO_NIC;
        } else {
            event->address = 0x10000000; 
            who = WHO_MEM;
        }

        _CPU_POWER_DBG("send a MEM event address=%#lx\n", event->address );


	/* set up counts */
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
	mycounts.L2_read=1; mycounts.L2_readmiss=0; mycounts.L2_write=1; mycounts.L2_writemiss=0; mycounts.L3_read=1; mycounts.L3_readmiss=0; mycounts.L3_write=1; mycounts.L3_writemiss=0;
	mycounts.L1Dir_read=1; mycounts.L1Dir_readmiss=0; mycounts.L1Dir_write=1; mycounts.L1Dir_writemiss=0; mycounts.L2Dir_read=1; mycounts.L2Dir_readmiss=0; mycounts.L2Dir_write=1;
        mycounts.L2Dir_writemiss=0;
	mycounts.memctrl_read=1; mycounts.memctrl_write=1;
	

	/* McPAT beta */
	pdata = power->getPower(current, CACHE_IL1, mycounts, 1);
	pdata = power->getPower(current, CACHE_DL1, mycounts, 1);  
	pdata = power->getPower(current, CACHE_ITLB, mycounts, 1); 
	pdata = power->getPower(current, CACHE_DTLB, mycounts, 1); 
	pdata = power->getPower(current, RF, mycounts, 1);
	pdata = power->getPower(current, IB, mycounts, 1);
	pdata = power->getPower(current, PIPELINE, mycounts, 1);
	pdata = power->getPower(current, BYPASS, mycounts, 1);
	pdata = power->getPower(current, EXEU_ALU, mycounts, 1);
	pdata = power->getPower(current, EXEU_FPU, mycounts, 1);
	pdata = power->getPower(current, LSQ, mycounts, 1);  
	pdata = power->getPower(current, BPRED, mycounts, 1); 
	pdata = power->getPower(current, SCHEDULER_U, mycounts, 1);
	pdata = power->getPower(current, RENAME_U, mycounts, 1);
	pdata = power->getPower(current, BTB, mycounts, 1);
	pdata = power->getPower(current, LOAD_Q, mycounts, 1);  
	pdata = power->getPower(current, CACHE_L1DIR, mycounts, 1);
	pdata = power->getPower(current, CACHE_L2DIR, mycounts, 1);
	pdata = power->getPower(current, CACHE_L2, mycounts, 1);
	pdata = power->getPower(current, CACHE_L3, mycounts, 1);		
	pdata = power->getPower(current, MEM_CTRL, mycounts, 1);
	pdata = power->getPower(current, ROUTER, mycounts, 1); 


	/* McPAT alpha 
	pdata = power->getPower(current, CACHE_IL1, "1:1:1:1:1:0", 1);  //<read hits>:<read misses>:<miss buf access>:<fill buf access>:<prefetch buf access>:<wbb buf access>	
	//pdata = power->getPower(current, CACHE_IL2, "0:0:1:1", 1);  	
	pdata = power->getPower(current, CACHE_DL1, "1:0:0:1:1:1", 1);  //<read hits>:<read misses>:<miss buf access>:<fill buf access>:<prefetch buf access>:<write_access> 
	//pdata = power->getPower(current, CACHE_DL2, "0:0:1:1", 1);  
	pdata = power->getPower(current, CACHE_ITLB, "2:0", 1); //<total hits>:<total misses>
	pdata = power->getPower(current, CACHE_DTLB, "1:1", 1); //<total hits>:<total misses> 
	pdata = power->getPower(current, RF, "2:2:2:2:2:2:2:2:2", 1); //<int_regfile_reads>:<int_regfile_writes>:<float_regfile_reads>:<float_regfile_writes>:<function_calls>:
									//<phy_int_regfile_reads>:<phy_int_regfile_writes>:<phy_float_regfile_reads>:<phy_float_regfile_writes>
	pdata = power->getPower(current, IB, "2:2", 1);  //<instruction_buffer_reads>:<instruction_buffer_writes>
	pdata = power->getPower(current, ISSUE_Q, "2:2", 1); //<instruction_window_reads>:<instruction_window_writes>   	  
	pdata = power->getPower(current, INST_DECODER, "2", 1);  //<total_instructions>
	pdata = power->getPower(current, PIPELINE, "none", 1);
	pdata = power->getPower(current, BYPASS, "2:2:2", 1);  //<bypassbus_access>:<int_instructions>:<fp_instructions> 
	pdata = power->getPower(current, LOGIC, "2:2:2", 1);  //<total_instructions>:<int_instructions>:<fp_instructions>
	pdata = power->getPower(current, EXEU_ALU, "2", 1); //<int_instructions>  
	pdata = power->getPower(current, EXEU_FPU, "2", 1);  //<fp_instructions>
	pdata = power->getPower(current, EXEU, "2", 1);  //<int_instructions> 
	pdata = power->getPower(current, LSQ, "1:1:1:1", 1);  //inorder--<lsq_access>  ooo--<load_buffer_reads>:<load_buffer_writes>:<store_buffer_reads>:<store_buffer_writes>
	pdata = power->getPower(current, BPRED, "2:2", 1); //<branch_instructions>:<branch_mispredictions>
	pdata = power->getPower(current, RAT, "2:2:2:2:2", 1); //<int_instructions>,<branch_mispredictions>,<branch_instructions>,<commited_instructions>,<fp_instructions> 
	pdata = power->getPower(current, ROB, "2:2", 1); //<ROB_reads>:<ROB_writes> 
	pdata = power->getPower(current, BTB, "2:2", 1); //<branch_instructions>:<branch_mispredictions> 	
	pdata = power->getPower(current, CACHE_L2, "1:1:1:1:1:1:1:1:1:1", 1);		
	//<read_accesses>:<write_accesses>:<miss_buffer_accesses>:<fill_buffer_accesses>:<prefetch_buffer_reads>:<prefetch_buffer_writes>:<wbb_reads>:<wbb_writes>
	//:<L2directory_read_accesses>:<L2directory_write_accesse>
	pdata = power->getPower(current, MEM_CTRL, "1:1", 1); //<mc_memory_reads>:<mc_memory_writes>
	pdata = power->getPower(current, ROUTER, "1", 1); //<total_router_accesses>
	pdata = power->getPower(current, CLOCK, "none", 1);
	*/

	/* sim-panalyzer
	pdata = power->getPower(current, CACHE_IL1, "0:198643:1:1", 1);  //"read(0)/write(1):address"latency"	
	pdata = power->getPower(current, CACHE_IL2, "0:0:1:1", 1);  	
	pdata = power->getPower(current, CACHE_DL1, "0:0:1:1", 1);  
	pdata = power->getPower(current, CACHE_DL2, "0:0:1:1", 1);  
	pdata = power->getPower(current, CACHE_ITLB, "0:0:1:1", 1); 
	pdata = power->getPower(current, CACHE_DTLB, "0:0:1:1", 1);  
	pdata = power->getPower(current, BPRED, "1", 1);  
	pdata = power->getPower(current, RF, "1", 1);  	  
	pdata = power->getPower(current, ALU, "1", 1);  
	pdata = power->getPower(current, FPU, "1", 1);  
	pdata = power->getPower(current, MULT, "1", 1);  
	pdata = power->getPower(current, LOGIC, "1", 1);  
	pdata = power->getPower(current, IO, "0:0:1:1", 1);  
	pdata = power->getPower(current, CLOCK, "1", 1);
	*/ 
        //_CPU_POWER_DBG("ID %lu : current power = %f pW\n", Id(), pdata.currentPower);
       // _CPU_POWER_DBG("ID %lu : total power = %f pW\n", Id(), pdata.totalEnergy); 
        //_CPU_POWER_DBG("ID %lu : peak power = %f pW\n", Id(), pdata.peak);
	//_CPU_POWER_DBG("ID %lu : current cycle = %d \n", Id(), pdata.currentCycle);
	regPowerStats(pdata);

        mem->Send( (Cycle_t)3, event );
        state = WAIT;


    } else {
        if ( ( event = static_cast< MemEvent* >( mem->Recv() ) ) ) {
            _CPU_POWER_DBG("got a MEM event address=%#lx\n", event->address );

	    //pdata = power->getPower(current, CACHE_IL1, "1:1:1:1:1:0", 1);  
            /*//pdata = power->getPower(current, CACHE_IL2, "1:0:1:1", 1);  
	    pdata = power->getPower(current, CACHE_DL1, "1:0:0:1:1:1", 1); 
	    //pdata = power->getPower(current, CACHE_DL2, "1:0:1:1", 1);
	    pdata = power->getPower(current, CACHE_ITLB, "2:0", 1);  
	    pdata = power->getPower(current, CACHE_DTLB, "1:1", 1);*/
	    /*sim-panalyzer
	    pdata = power->getPower(current, CACHE_IL1, "1:0:1:1", 1);  //"read(0)/write(1):address"latency"
            pdata = power->getPower(current, CACHE_IL2, "1:0:1:1", 1);  
	    pdata = power->getPower(current, CACHE_DL1, "1:0:1:1", 1); 
	    pdata = power->getPower(current, CACHE_DL2, "1:0:1:1", 1);
	    pdata = power->getPower(current, CACHE_ITLB, "1:0:1:1", 1);  
	    pdata = power->getPower(current, CACHE_DTLB, "1:0:1:1", 1);  
	    */
            //_CPU_POWER_DBG("ID %lu : current power = %f pW\n", Id(), pdata.currentPower);
            //_CPU_POWER_DBG("ID %lu : total power = %f pW\n", Id(), pdata.totalEnergy);
	    //_CPU_POWER_DBG("ID %lu : peak power = %f pW\n", Id(), pdata.peak); 
            //_CPU_POWER_DBG("ID %lu : current cycle = %d\n", Id(), pdata.currentCycle);
	    //regPowerStats(pdata);

            state = SEND;
	   
        }
    }
    return false;
}

extern "C" {
Cpu_power* cpu_powerAllocComponent( SST::ComponentId_t id, 
                                    SST::Component::Params_t& params )
{
    return new Cpu_power( id, params );
}
}

#if WANT_CHECKPOINT_SUPPORT2
BOOST_CLASS_EXPORT(Cpu_power)

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Cpu_power, bool, SST::Cycle_t, SST::Time_t )
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Cpu_power, bool, SST::Cycle_t)
#endif

