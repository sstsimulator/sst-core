#ifndef _SST_POWER_H
#define _SST_POWER_H

#include <stdlib.h>
#include <stdio.h>			
#include <unistd.h>
#include <math.h>
#include <string>
#include <map>
#include <sst/component.h>
#include <sst/debug.h>
#include <sst/sst.h>
#include <sst/boost.h>


/*********Sim-Panalyzer************/
//#define PANALYZER_H
////#define LV1_PANALYZER_H
//#define LV2_PANALYZER_H

/* added for sim-panalyzer power analysis */
extern "C"{
#ifdef PANALYZER_H

#ifdef LV1_PANALYZER_H
#include "../../sst/techModels/libsim-panalyzer/lv1_opts.h"
#include "../../sst/techModels/libsim-panalyzer/lv1_panalyzer.h"
#include "../../sst/techModels/libsim-panalyzer/lv1_cache_panalyzer.h"
#include "../../sst/techModels/libsim-panalyzer/io_panalyzer.h"
#endif

#ifdef LV2_PANALYZER_H
#include "../../sst/techModels/libsim-panalyzer/technology.h"
#include "../../sst/techModels/libsim-panalyzer/alu_panalyzer.h"
#include "../../sst/techModels/libsim-panalyzer/mult_panalyzer.h"
#include "../../sst/techModels/libsim-panalyzer/fpu_panalyzer.h"
#include "../../sst/techModels/libsim-panalyzer/uarch_panalyzer.h"
#endif //lv2_panalyzer_h

#endif //panalyzer_h
}


/***********McPAT05****************/
//#define McPAT05_H

/*added for McPAT05 power analysis */
#ifdef McPAT05_H
#include "../../sst/techModels/libMcPAT/io.h"
#include "../../sst/techModels/libMcPAT/logic.h"
#include "../../sst/techModels/libMcPAT/full_decoder.h"
#include "../../sst/techModels/libMcPAT/crossbarswitch.h"
#include "../../sst/techModels/libMcPAT/basic_circuit.h"
#include "../../sst/techModels/libMcPAT/processor.h"
#endif //mcpat05_h


/***********McPAT06****************/
#define McPAT06_H

/*added for McPAT06 power analysis */
#ifdef McPAT06_H
#include "../../sst/techModels/libMcPATbeta/io.h"
#include "../../sst/techModels/libMcPATbeta/logic.h"
//#include "../../sst/techModels/libMcPATbeta/full_decoder.h"
//#include "../../sst/techModels/libMcPATbeta/crossbarswitch.h"
#include "../../sst/techModels/libMcPATbeta/basic_circuit.h"
#include "../../sst/techModels/libMcPATbeta/processor.h"
#endif //mcpat06_h


namespace SST {

        typedef int nm;
        typedef int ns;

	enum logic_style {STATIC, DYNAMIC};
	enum clock_style{NORM_H, BALANCED_H};
	enum io_style{IN, OUT, BI};
	enum topology_style{TWODMESH, RING, CROSSBAR}; 

	//TODO consider to add "ALL" to ptype, so users don't have to list/decouple power params one by one.
	enum ptype {CACHE_IL1, CACHE_IL2, CACHE_DL1, CACHE_DL2, CACHE_ITLB, CACHE_DTLB, CLOCK, BPRED, RF, IO, LOGIC, EXEU_ALU, EXEU_FPU, MULT, IB, ISSUE_Q, INST_DECODER, BYPASS, EXEU, PIPELINE, LSQ, RAT, ROB, BTB, CACHE_L2, MEM_CTRL, ROUTER, LOAD_Q, RENAME_U, SCHEDULER_U, CACHE_L3, CACHE_L1DIR, CACHE_L2DIR, UARCH}; //RS is renamed by ISSUE_Q because of name collision	in s-p
	enum pmodel{McPAT, SimPanalyzer, McPAT05, MySimpleModel}; //McPAT05 is older version

	
	

typedef struct 
{
	watts il1_read, il1_write;
	watts il2_read, il2_write;
	watts dl1_read, dl1_write;
	watts dl2_read, dl2_write;
	watts itlb_read, itlb_write;
	watts dtlb_read, dtlb_write;
	watts aio, dio;
	watts clock;
	watts logic;
	watts rf, bpred;
	watts alu, fpu, mult, exeu, lsq;
	watts uarch;
}Punit_t;

typedef struct  //this takes over the core params in each subcomponent_params_t
{
	unsigned core_physical_address_width, core_temperature, core_tech_node;
	unsigned core_virtual_address_width, core_virtual_memory_page_size, core_number_hardware_threads;		
	unsigned machine_bits, archi_Regs_IRF_size, archi_Regs_FRF_size, core_phy_Regs_IRF_size, core_phy_Regs_FRF_size;
	unsigned core_register_windows_size, core_opcode_width;	
	unsigned core_instruction_window_size;
	unsigned core_issue_width, core_decode_width, core_fetch_width, core_commit_width;	
	unsigned core_instruction_length;
	unsigned core_instruction_buffer_size;
	unsigned ALU_per_core, FPU_per_core;	
	unsigned core_store_buffer_size, core_memory_ports;
	unsigned core_int_pipeline_depth, core_RAS_size, core_ROB_size, core_load_buffer_size, core_number_of_NoCs;
	unsigned core_number_instruction_fetch_ports, core_fp_issue_width, core_fp_instruction_window_size;	
} core_params_t;

typedef struct
{
	farads unit_scap;  //switching
        farads unit_icap;  //internal
        farads unit_lcap;  //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	unsigned core_physical_address_width;
	unsigned core_virtual_address_width, core_virtual_memory_page_size, core_number_hardware_threads;
	unsigned core_temperature, core_tech_node;  

	//for cache components
	int num_sets;
	int line_size;
	int num_bitlines;
	int num_wordlines;
	int assoc;
	unsigned num_rwports, num_rports, num_wports, num_banks;
	double throughput, latency;
	unsigned miss_buf_size, fill_buf_size, prefetch_buf_size, wbb_buf_size;
	unsigned number_entries;
	unsigned device_type, directory_type;
} cache_params_t;



struct clock_params_t
{
	farads unit_scap;  //switching
	farads unit_icap;  //internal
	farads unit_lcap;   //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	unsigned core_temperature, core_tech_node;	

	//for clock components
	clock_style clk_style;
	double skew;
	nm chip_area;
	farads node_cap;
	int opt_clock_buffer_num;
};

struct bpred_params_t
{
	farads unit_icap; //internal
	farads unit_ecap;  //effective capacitance-for lv1
	farads unit_scap;  //switching
	volts vss;
	double op_freq;

	unsigned global_predictor_bits, global_predictor_entries, prediction_width, local_predictor_size, local_predictor_entries;
	unsigned chooser_predictor_bits, chooser_predictor_entries;
	unsigned nrows, ncols;
	unsigned num_rwports, num_rports, num_wports;	
	unsigned long bpred_access;


};

struct rf_params_t
{
	farads unit_icap; //internal
	farads unit_ecap;  //effective capacitance-for lv1
	farads unit_scap;  //switching
	volts vss;
	unsigned machine_bits, archi_Regs_IRF_size, archi_Regs_FRF_size;
	unsigned core_issue_width, core_register_windows_size, core_number_hardware_threads, core_opcode_width, core_virtual_address_width;
	unsigned core_temperature, core_tech_node;

	double op_freq;
	unsigned nrows, ncols;
	unsigned num_rwports, num_rports, num_wports;   
	unsigned long rf_access;
};

struct io_params_t
{
	farads unit_scap;      //switching
	farads unit_icap;  //internal
	farads unit_lcap;   //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	
	//for I/O components
	io_style i_o_style;
	unsigned opt_io_buffer_num;
	double ustrip_len;
	unsigned bus_width;   //bus width
	unsigned bus_size;    //io transaction size = buffer size = bsize in io_panalyzer
	unsigned io_access_time;  //in cycles
	unsigned io_cycle_time;  // in cycles
};

struct logic_params_t
{
	farads unit_scap;      //switching
	farads unit_icap;  //internal
	farads unit_lcap;   //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;
	unsigned core_instruction_window_size, core_issue_width, core_number_hardware_threads;
	unsigned core_decode_width, archi_Regs_IRF_size, archi_Regs_FRF_size;
	unsigned core_temperature, core_tech_node;

	//for logic components
	logic_style lgc_style; //Dynamic/Static Logic	
	unsigned num_gates;
	unsigned num_functions;   // Does this mean and, or, xor??
	unsigned num_fan_in;
	unsigned num_fan_out;
};

struct other_params_t
{
	farads unit_scap;      //switching
	farads unit_icap;  //internal
	farads unit_lcap;   //leakage
	farads unit_ecap;  //effective capacitance-for lv1
	volts vss;
	double op_freq;

};


struct ib_params_t
{
	unsigned core_instruction_length, core_issue_width, core_number_hardware_threads;
	unsigned core_instruction_buffer_size, num_rwports;
	unsigned core_temperature, core_tech_node, core_virtual_address_width, core_virtual_memory_page_size;
};

struct irs_params_t
{
	unsigned core_number_hardware_threads, core_instruction_length, core_instruction_window_size;
	unsigned core_issue_width;
	unsigned core_temperature, core_tech_node;
};

struct decoder_params_t
{
	unsigned core_opcode_width;
	unsigned core_temperature, core_tech_node;
};

struct bypass_params_t
{
	unsigned core_number_hardware_threads, ALU_per_core, machine_bits, FPU_per_core;
	unsigned core_opcode_width, core_virtual_address_width;
	unsigned core_store_buffer_size, core_memory_ports;
	unsigned core_temperature, core_tech_node;
};

struct pipeline_params_t
{
	unsigned core_number_hardware_threads, core_fetch_width, core_decode_width;
	unsigned core_issue_width, core_commit_width, core_instruction_length, core_virtual_address_width;
	unsigned core_opcode_width, core_int_pipeline_depth;
	unsigned machine_bits, archi_Regs_IRF_size;
	unsigned core_temperature, core_tech_node;
};

typedef struct
{
	farads unit_scap;  //switching
	volts vss;
	double op_freq;

	//for btb components
	
	int line_size, assoc;
	unsigned num_banks;
	double throughput, latency;
} btb_params_t;

typedef struct
{
	double mc_clock;
	unsigned llc_line_length, databus_width, addressbus_width, req_window_size_per_channel;
	unsigned memory_channels_per_mc, IO_buffer_size_per_channel;
	unsigned memory_number_ranks, memory_peak_transfer_rate;
} mc_params_t;

typedef struct
{
	double clockrate;
	unsigned has_global_link, flit_bits, input_buffer_entries_per_vc, virtual_channel_per_port, input_ports;
	unsigned output_ports, link_throughput, link_latency, horizontal_nodes, vertical_nodes;
	topology_style topology;
} router_params_t;


typedef struct
{
	/*McPAT*/
	double branch_read, branch_write, RAS_read, RAS_write;
	double il1_read, il1_readmiss, IB_read, IB_write, BTB_read, BTB_write;
	double int_win_read, int_win_write, fp_win_read, fp_win_write, ROB_read, ROB_write;
	double iFRAT_read, iFRAT_write, iFRAT_search, fFRAT_read, fFRAT_write, fFRAT_search, iRRAT_write, fRRAT_write;
	double ifreeL_read, ifreeL_write, ffreeL_read, ffreeL_write, idcl_read, fdcl_read;
	double dl1_read, dl1_readmiss, dl1_write, dl1_writemiss, LSQ_read, LSQ_write;
	double itlb_read, itlb_readmiss, dtlb_read, dtlb_readmiss;
	double int_regfile_reads, int_regfile_writes, float_regfile_reads, float_regfile_writes, RFWIN_read, RFWIN_write;
	double bypass_access;
	double router_access;
	double L2_read, L2_readmiss, L2_write, L2_writemiss, L3_read, L3_readmiss, L3_write, L3_writemiss;
	double L1Dir_read, L1Dir_readmiss, L1Dir_write, L1Dir_writemiss, L2Dir_read, L2Dir_readmiss, L2Dir_write, L2Dir_writemiss;
	double memctrl_read, memctrl_write;
	/*S-P*/
	double il1_ReadorWrite, il1_accessaddress, il1_datablock, il1_latency, il1_access;
	double il2_ReadorWrite, il2_accessaddress, il2_datablock, il2_latency, il2_access;
	double dl1_ReadorWrite, dl1_accessaddress, dl1_datablock, dl1_latency, dl1_access;
	double dl2_ReadorWrite, dl2_accessaddress, dl2_datablock, dl2_latency, dl2_access;
	double itlb_ReadorWrite, itlb_accessaddress, itlb_datablock, itlb_latency, itlb_access;
	double dtlb_ReadorWrite, dtlb_accessaddress, dtlb_datablock, dtlb_latency, dtlb_access;
	double bpred_access, rf_access, alu_access, fpu_access, mult_access, logic_access, clock_access; 
	double io_ReadorWrite, io_accessaddress, io_datablock, io_latency, io_access;
} usagecounts_t;



class Power{
   public:
 	Pdissipation_t p_usage_cache_il1;
	Pdissipation_t p_usage_cache_il2;
	Pdissipation_t p_usage_cache_dl1;
	Pdissipation_t p_usage_cache_dl2;
	Pdissipation_t p_usage_cache_itlb;
	Pdissipation_t p_usage_cache_dtlb;
	Pdissipation_t p_usage_clock;
	Pdissipation_t p_usage_io;
	Pdissipation_t p_usage_logic;
	Pdissipation_t p_usage_alu;
	Pdissipation_t p_usage_fpu;
	Pdissipation_t p_usage_mult;	
	Pdissipation_t p_usage_rf;
	Pdissipation_t p_usage_bpred;
	Pdissipation_t p_usage_ib;
	Pdissipation_t p_usage_rs;
	Pdissipation_t p_usage_decoder;
	Pdissipation_t p_usage_bypass;
	Pdissipation_t p_usage_exeu;
	Pdissipation_t p_usage_pipeline;
	Pdissipation_t p_usage_lsq;
	Pdissipation_t p_usage_rat;
	Pdissipation_t p_usage_rob;
	Pdissipation_t p_usage_btb;
	Pdissipation_t p_usage_cache_l2;
	Pdissipation_t p_usage_mc;
	Pdissipation_t p_usage_router;
	Pdissipation_t p_usage_loadQ;
	Pdissipation_t p_usage_renameU;
	Pdissipation_t p_usage_schedulerU;
	Pdissipation_t p_usage_cache_l3;
	Pdissipation_t p_usage_cache_l1dir;
	Pdissipation_t p_usage_cache_l2dir;
	Pdissipation_t p_usage_uarch; //"ALL"

	cache_params_t cache_il1_tech;
	cache_params_t cache_il2_tech;
	cache_params_t cache_dl1_tech;
	cache_params_t cache_dl2_tech;
	cache_params_t cache_itlb_tech;
	cache_params_t cache_dtlb_tech;
	cache_params_t cache_l2_tech;
	cache_params_t cache_l3_tech;
	cache_params_t cache_l1dir_tech;
	cache_params_t cache_l2dir_tech;
	clock_params_t clock_tech;
	bpred_params_t bpred_tech;
	rf_params_t rf_tech;
	io_params_t io_tech;
	logic_params_t logic_tech;
	other_params_t alu_tech;
	other_params_t fpu_tech;
	other_params_t mult_tech;
	other_params_t uarch_tech;
	ib_params_t ib_tech;
	irs_params_t irs_tech;
	bypass_params_t bypass_tech;
	decoder_params_t decoder_tech;
	pipeline_params_t pipeline_tech;
	core_params_t core_tech;	
	btb_params_t btb_tech;
	mc_params_t mc_tech;
	router_params_t router_tech;

	float clockRate; //frequency McPAT
	ComponentId_t p_compID;
	int p_powerLevel; //level 1: v, f, sC, iC, lC; level 2: v, f, sC, and other params
	bool p_powerMonitor; // if a component want to have power monitored
	pmodel p_powerModel;
	Punit_t p_unitPower; // stores unit power per sub-component access
	I p_meanPeak, p_meanPeakAll; // for manual error bar on mean peak power
	double p_areaMcPAT;
	unsigned p_numL2, p_machineType; //1: inorder, 0:ooo
	char *p_McPATxmlpath;
	bool p_ifReadEntireXML, p_ifGetMcPATUnitP;

	// sim-panalyzer parameters
	#ifdef LV2_PANALYZER_H
	#ifdef CACHE_PANALYZER_H
	fu_cache_pspec_t *il1_pspec;
	fu_cache_pspec_t *il2_pspec;
	fu_cache_pspec_t *dl1_pspec;
	fu_cache_pspec_t *dl2_pspec;
	fu_cache_pspec_t *itlb_pspec;
	fu_cache_pspec_t *dtlb_pspec;
	#endif /* CACHE_PANALYZER_H */
	#ifdef CLOCK_PANALYZER_H
	fu_clock_pspec_t *clock_pspec;
	#endif /* CLOCK_PANALYZER_H */
	#ifdef MEMORY_PANALYZER_H
	fu_sbank_pspec_t *rf_pspec;
	fu_sbank_pspec_t *bpred_pspec;
	#endif /* MEMORY_PANALYZER_H */
	#ifdef LOGIC_PANALYZER_H 
	fu_logic_pspec_t *logic_pspec;
	#endif  /*LOGIC_PANALYZER_H*/	
	fu_alu_pspec_t *alu_pspec;
	fu_mult_pspec_t *mult_pspec;
	fu_fpu_pspec_t *fpu_pspec;
	#endif /*lv2_panalyzer*/
	
	#ifdef IO_PANALYZER_H
	/* address io panalyzer */
	fu_io_pspec_t *aio_pspec;
	/* data io panalyzer */
	fu_io_pspec_t *dio_pspec;
	#endif /* IO_PANALYZER_H */

	// McPAT05 parameters
	int perThreadState;
	double C_EXEU; //exeu capacitance

	#ifdef McPAT05_H
	#ifdef  XML_PARSE_H_
	ParseXML *p_Mp1;
	#endif /*  XML_PARSE_H_ */
	#ifdef  PROCESSOR_H_
	Processor p_Mproc;
	InputParameter interface_ip;
	#endif /* PROCESSOR_H_*/
	#ifdef  CORE_H_
	tlb_core itlb, dtlb;
	cache_processor icache, dcache;
	BTB_core BTB;
	RF_core	 IRF, FRF, RFWIN, phyIRF, phyFRF;
	IB_core  IB;
	RS_core	 iRS, iISQ,fRS, fISQ;
	LSQ_core LSQ, loadQ;
	resultbus int_bypass,intTagBypass, fp_bypass, fpTagBypass;
	selection_logic instruction_selection;
	dep_resource_conflict_check idcl,fdcl;
	full_decoder inst_decoder;
	core_pipeline corepipe;
	UndifferentiatedCore undifferentiatedCore;
	MCclock_network clockNetwork;
	ROB_core ROB;	
	predictor_core predictor;
	RAT_core iRRAT,iFRAT,iFRATCG, fRRAT,fFRAT,fFRATCG;
	#endif /* CORE_H_*/
	#ifdef  SHAREDCACHE_H_
	cache_processor llCache,directory;
        pipeline pipeLogicCache, pipeLogicDirectory;
        MCclock_network L2clockNetwork;
	#endif /*  SHAREDCACHE_H_*/
	#ifdef MEMORYCTRL_H_
	selection_logic MC_arb;
        cache_processor frontendBuffer,readBuffer, writeBuffer;
	pipeline MCpipeLogic;
        MCclock_network MCclockNetwork;
        MCBackend transecEngine;
        MCPHY	  PHY;
	#endif /*MEMORYCTRL_H_*/
	#ifdef ROUTER_H_
	cache_processor inputBuffer, routingTable;
        crossbarswitch xbar;
        Arbiter vcAllocatorStage1,vcAllocatorStage2, switchAllocatorStage1, switchAllocatorStage2;
        wire globalInterconnect;
        pipeline RTpipeLogic;
        MCclock_network RTclockNetwork;
	#endif /*ROUTER_H_*/
	#endif /*McPAT05_H*/


	//McPAT06 parameters
	#ifdef McPAT06_H
	#ifdef  XML_PARSE_H_
	ParseXML *p_Mp1;
	#endif /*  XML_PARSE_H_ */
	#ifdef  PROCESSOR_H_
	Processor p_Mproc;
	Core *p_Mcore;
	SharedCache* l2array;
        SharedCache* l3array;
        SharedCache* l1dirarray;
        SharedCache* l2dirarray;
        NoC* nocs;
        MemoryController* mc;
	#endif /* PROCESSOR_H_*/
	#ifdef  CORE_H_
	InstFetchU * ifu;
	LoadStoreU * lsu;
	MemManU    * mmu;
	EXECU      * exu;
	RENAMINGU  * rnu;
        Pipeline   * corepipe;
        UndiffCore * undiffCore;
        CoreDynParam  coredynp;
	
	ArrayST * globalBPT;
	ArrayST * L1_localBPT;
	ArrayST * L2_localBPT;
	ArrayST * chooser;
	ArrayST * RAS;
	
	InstCache icache;
	ArrayST * IB;
	ArrayST * BTB;
	BranchPredictor * BPT;

	ArrayST         * int_inst_window;
	ArrayST         * fp_inst_window;
	ArrayST         * ROB;
        selection_logic * instruction_selection;

	ArrayST * iFRAT;
	ArrayST * fFRAT;
	ArrayST * iRRAT;
	ArrayST * fRRAT;
	ArrayST * ifreeL;
	ArrayST * ffreeL;
	dep_resource_conflict_check * idcl;
	dep_resource_conflict_check * fdcl;

	DataCache dcache;
	ArrayST * LSQ;//it is actually the store queue but for inorder processors it serves as both loadQ and StoreQ
	ArrayST * LoadQ;

	ArrayST * itlb;
	ArrayST * dtlb;

	ArrayST * IRF;
	ArrayST * FRF;
	ArrayST * RFWIN;

	RegFU          * rfu;
	SchedulerU     * scheu;
        FunctionalUnit * fp_u;
        FunctionalUnit * exeu;
	McPATComponent  bypass;
	interconnect * int_bypass;
	interconnect * intTagBypass;
	interconnect * fp_bypass;
	interconnect * fpTagBypass;
	#endif /* CORE_H_*/
	#endif /*McPAT06_H*/


   public:
       	//Default constructor
	Power();
        Power(ComponentId_t compID) {
            PowerInit(compID);}
        void PowerInit(ComponentId_t compID) {
	    clockRate = 2200000000.0;
            p_compID = compID;
	    p_powerLevel = 1; p_powerMonitor = false; p_powerModel = McPAT;  // This setting is important since params are not read in order as they apprears
	    p_meanPeak = p_meanPeakAll = 0.0;
	    p_areaMcPAT = 0.0; p_machineType = 1; p_numL2 = 1;
	    p_ifReadEntireXML = p_ifGetMcPATUnitP = false;
	    //p_McPATonPipe = p_McPATonIRS = p_McPATonRF = false;  //if xxx has power estimated by McPAT already
	    #ifdef McPAT05_H
	    McPAT05initBasic(); //initialize basic InputParameter interface_ip
	    #endif

            // power usage initialization
	    memset(&p_usage_cache_il1,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_il2,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_dl1,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_dl2,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_itlb,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_dtlb,0,sizeof(Pdissipation_t));
	    memset(&p_usage_clock,0,sizeof(Pdissipation_t));
	    memset(&p_usage_io,0,sizeof(Pdissipation_t));
	    memset(&p_usage_logic,0,sizeof(Pdissipation_t));
	    memset(&p_usage_alu,0,sizeof(Pdissipation_t));
	    memset(&p_usage_fpu,0,sizeof(Pdissipation_t));
	    memset(&p_usage_mult,0,sizeof(Pdissipation_t));
	    memset(&p_usage_uarch,0,sizeof(Pdissipation_t));
	    memset(&p_usage_rf,0,sizeof(Pdissipation_t));
	    memset(&p_usage_bpred,0,sizeof(Pdissipation_t));
	    memset(&p_usage_ib,0,sizeof(Pdissipation_t));
	    memset(&p_usage_rs,0,sizeof(Pdissipation_t));
	    memset(&p_usage_decoder,0,sizeof(Pdissipation_t));
	    memset(&p_usage_bypass,0,sizeof(Pdissipation_t));
	    memset(&p_usage_exeu,0,sizeof(Pdissipation_t));
	    memset(&p_usage_pipeline,0,sizeof(Pdissipation_t));
	    memset(&p_usage_lsq,0,sizeof(Pdissipation_t));
	    memset(&p_usage_rat,0,sizeof(Pdissipation_t));
	    memset(&p_usage_rob,0,sizeof(Pdissipation_t));
	    memset(&p_usage_btb,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_l2,0,sizeof(Pdissipation_t));
	    memset(&p_usage_mc,0,sizeof(Pdissipation_t));
	    memset(&p_usage_router,0,sizeof(Pdissipation_t));
	    memset(&p_usage_loadQ,0,sizeof(Pdissipation_t));
	    memset(&p_usage_renameU,0,sizeof(Pdissipation_t));
	    memset(&p_usage_schedulerU,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_l3,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_l1dir,0,sizeof(Pdissipation_t));
	    memset(&p_usage_cache_l2dir,0,sizeof(Pdissipation_t));
	    memset(&p_unitPower,0,sizeof(Punit_t));
            // cache params initilaization
            /* cache_il1 */
            cache_il1_tech.unit_scap = 32768.0; cache_il1_tech.vss = 0.0; cache_il1_tech.op_freq = 0; cache_il1_tech.num_sets = 0;	    
            cache_il1_tech.line_size = 32; cache_il1_tech.num_bitlines = 0; cache_il1_tech.num_wordlines = 0; cache_il1_tech.assoc = 8;
	    cache_il1_tech.unit_icap = 0.0; cache_il1_tech.unit_lcap = 0.0; cache_il1_tech.unit_ecap = 0;
	    cache_il1_tech.num_rwports = cache_il1_tech.num_rports = cache_il1_tech.num_wports = 0; cache_il1_tech.num_banks = 1;
	    cache_il1_tech.throughput = 3.0; cache_il1_tech.latency = 3.0; cache_il1_tech.core_physical_address_width = 52;
	    cache_il1_tech.miss_buf_size = cache_il1_tech.fill_buf_size = cache_il1_tech.prefetch_buf_size = 16; cache_il1_tech.wbb_buf_size = 0;
	    cache_il1_tech.core_virtual_address_width = 64; cache_il1_tech.core_virtual_memory_page_size = 4096; cache_il1_tech.core_number_hardware_threads = 2;	
	    cache_il1_tech.number_entries = 0; cache_il1_tech.core_temperature=360; cache_il1_tech.core_tech_node=32;  cache_il1_tech.device_type = 0; 
	    cache_il1_tech.directory_type = 1;  
            /* cache_il2 */
            cache_il2_tech.unit_scap = 0.0; cache_il2_tech.vss = 0.0; cache_il2_tech.op_freq = 0; cache_il2_tech.num_sets = 0;
            cache_il2_tech.line_size = 0; cache_il2_tech.num_bitlines = 0; cache_il2_tech.num_wordlines = 0; cache_il2_tech.assoc = 0;
	    cache_il2_tech.unit_icap = 0.0; cache_il2_tech.unit_lcap = 0.0; cache_il2_tech.unit_ecap = 0;
	    cache_il2_tech.num_rwports = cache_il2_tech.num_rports = cache_il2_tech.num_wports = cache_il2_tech.num_banks = 0;
	    cache_il2_tech.throughput = cache_il2_tech.latency = 0.0; cache_il2_tech.core_physical_address_width = 0;
	    cache_il2_tech.miss_buf_size = cache_il2_tech.fill_buf_size = cache_il2_tech.prefetch_buf_size = 0; cache_il2_tech.wbb_buf_size = 0;
	    cache_il2_tech.core_virtual_address_width = cache_il2_tech.core_virtual_memory_page_size = cache_il2_tech.core_number_hardware_threads = 0;	
	    cache_il2_tech.number_entries = 0; cache_il2_tech.core_temperature=360; cache_il2_tech.core_tech_node=65; cache_il2_tech.device_type = 0;  	  
	    cache_il2_tech.directory_type = 1;  
            /* cache_dl1 */
            cache_dl1_tech.unit_scap = 16384.0; cache_dl1_tech.vss = 0.0; cache_dl1_tech.op_freq = 0; cache_dl1_tech.num_sets = 0;
            cache_dl1_tech.line_size = 32; cache_dl1_tech.num_bitlines = 0; cache_dl1_tech.num_wordlines = 0; cache_dl1_tech.assoc = 8;
	    cache_dl1_tech.unit_icap = 0.0; cache_dl1_tech.unit_lcap = 0.0; cache_dl1_tech.unit_ecap = 0;
	    cache_dl1_tech.num_rwports = cache_dl1_tech.num_rports = cache_dl1_tech.num_wports = cache_dl1_tech.num_banks = 1;
	    cache_dl1_tech.throughput =3.0; cache_dl1_tech.latency = 3.0; cache_dl1_tech.core_physical_address_width = 52;
	    cache_dl1_tech.miss_buf_size = cache_dl1_tech.fill_buf_size = cache_dl1_tech.prefetch_buf_size = cache_dl1_tech.wbb_buf_size = 16;
	    cache_dl1_tech.core_virtual_address_width = cache_dl1_tech.core_virtual_memory_page_size = cache_dl1_tech.core_number_hardware_threads = 0;	
	    cache_dl1_tech.number_entries = 0; cache_dl1_tech.core_temperature=360; cache_dl1_tech.core_tech_node=32; cache_dl1_tech.device_type = 0;
	    cache_dl1_tech.directory_type = 1;	    
            /* cache_dl2 */
            cache_dl2_tech.unit_scap = 0.0; cache_dl2_tech.vss = 0.0; cache_dl2_tech.op_freq = 0; cache_dl2_tech.num_sets = 0;
            cache_dl2_tech.line_size = 0; cache_dl2_tech.num_bitlines = 0; cache_dl2_tech.num_wordlines = 0; cache_dl2_tech.assoc = 0;
	    cache_dl2_tech.unit_icap = 0.0; cache_dl2_tech.unit_lcap = 0.0; cache_dl2_tech.unit_ecap = 0;
	    cache_dl2_tech.num_rwports = cache_dl2_tech.num_rports = cache_dl2_tech.num_wports = cache_dl2_tech.num_banks = 0;
	    cache_dl2_tech.throughput = cache_dl2_tech.latency = 0.0; cache_dl2_tech.core_physical_address_width = 0;
	    cache_dl2_tech.miss_buf_size = cache_dl2_tech.fill_buf_size = cache_dl2_tech.prefetch_buf_size = 0; cache_dl2_tech.wbb_buf_size = 0;
	    cache_dl2_tech.core_virtual_address_width = cache_dl2_tech.core_virtual_memory_page_size = cache_dl2_tech.core_number_hardware_threads = 0;	
	    cache_dl2_tech.number_entries = 0; cache_dl2_tech.core_temperature=360; cache_dl2_tech.core_tech_node=65; cache_dl2_tech.device_type = 0;
	    cache_dl2_tech.directory_type = 1;  	    
            /* cache_itlb */
            cache_itlb_tech.unit_scap = 0.0; cache_itlb_tech.vss = 0.0; cache_itlb_tech.op_freq = 0; cache_itlb_tech.num_sets = 0;
            cache_itlb_tech.line_size = 0; cache_itlb_tech.num_bitlines = 0; cache_itlb_tech.num_wordlines = 0; cache_itlb_tech.assoc = 0;
	    cache_itlb_tech.unit_icap = 0.0; cache_itlb_tech.unit_lcap = 0.0; cache_itlb_tech.unit_ecap = 0;
	    cache_itlb_tech.num_rwports = cache_itlb_tech.num_rports = cache_itlb_tech.num_wports = cache_itlb_tech.num_banks = 0;
	    cache_itlb_tech.throughput = cache_itlb_tech.latency = 0.0; cache_itlb_tech.core_physical_address_width = 0;
	    cache_itlb_tech.miss_buf_size = cache_itlb_tech.fill_buf_size = cache_itlb_tech.prefetch_buf_size = 0; cache_itlb_tech.wbb_buf_size = 0;	
	    cache_itlb_tech.core_virtual_address_width = 64; cache_itlb_tech.core_virtual_memory_page_size = 4096; 
	    cache_itlb_tech.core_number_hardware_threads = 2; cache_itlb_tech.core_physical_address_width = 52; cache_itlb_tech.number_entries = 128;   
	    cache_itlb_tech.core_temperature=360; cache_itlb_tech.core_tech_node=32;  cache_itlb_tech.device_type = 0;
	    cache_itlb_tech.directory_type = 1;
            /* cache_dtlb */
            cache_dtlb_tech.unit_scap = 0.0; cache_dtlb_tech.vss = 0.0; cache_dtlb_tech.op_freq = 0; cache_dtlb_tech.num_sets = 0;
            cache_dtlb_tech.line_size = 0; cache_dtlb_tech.num_bitlines = 0; cache_dtlb_tech.num_wordlines = 0; cache_dtlb_tech.assoc = 0;
	    cache_dtlb_tech.unit_icap = 0.0; cache_dtlb_tech.unit_lcap = 0.0; cache_dtlb_tech.unit_ecap = 0;
	    cache_dtlb_tech.num_rwports = cache_dtlb_tech.num_rports = cache_dtlb_tech.num_wports = cache_dtlb_tech.num_banks = 0;
	    cache_dtlb_tech.throughput = cache_dtlb_tech.latency = 0.0; cache_dtlb_tech.core_physical_address_width = 0;
	    cache_dtlb_tech.miss_buf_size = cache_dtlb_tech.fill_buf_size = cache_dtlb_tech.prefetch_buf_size = 0; cache_dtlb_tech.wbb_buf_size = 0;	  
	    cache_dtlb_tech.core_virtual_address_width = 64; cache_dtlb_tech.core_virtual_memory_page_size = 4096; 
	    cache_dtlb_tech.core_number_hardware_threads = 2; cache_dtlb_tech.core_physical_address_width = 52; cache_dtlb_tech.number_entries = 128; 
	    cache_dtlb_tech.core_temperature=360; cache_dtlb_tech.core_tech_node=32; cache_dtlb_tech.device_type = 0; 
	    cache_dtlb_tech.directory_type = 1;
            /*clock*/
	    clock_tech.unit_scap=0.0; clock_tech.unit_icap=0.0; clock_tech.unit_lcap=0.0; clock_tech.vss=0.0; 
	    clock_tech.op_freq=0; clock_tech.clk_style=NORM_H; clock_tech.skew=0.0; clock_tech.chip_area=0; 
	    clock_tech.node_cap=0.0; clock_tech.opt_clock_buffer_num=0; clock_tech.unit_ecap=0.0;
	    clock_tech.core_temperature=360; clock_tech.core_tech_node=65;
	    /*bpred*/
	    bpred_tech.unit_icap=0.0; bpred_tech.unit_ecap=0.0; bpred_tech.vss=0.0;
	    bpred_tech.op_freq=0; bpred_tech.unit_scap=0.0; bpred_tech.bpred_access=0; bpred_tech.nrows=0; bpred_tech.ncols=0;
	    bpred_tech.num_rwports = bpred_tech.num_rports = bpred_tech.num_wports = 0;	  
	    bpred_tech.global_predictor_bits=2; bpred_tech.global_predictor_entries=4096; bpred_tech.prediction_width=1; bpred_tech.local_predictor_size=10;
	    bpred_tech.local_predictor_entries=1024; bpred_tech.chooser_predictor_bits=2; bpred_tech.chooser_predictor_entries=4096;  
	    /*rf*/
	    rf_tech.unit_scap=0.0; rf_tech.unit_icap=0.0; rf_tech.unit_ecap=0.0; rf_tech.vss=0.0;
	    rf_tech.op_freq=0; rf_tech.rf_access=0; rf_tech.nrows=0; rf_tech.ncols=0;
	    rf_tech.num_rwports = rf_tech.num_rports = rf_tech.num_wports = 0;	
	    rf_tech.machine_bits = 64; rf_tech.archi_Regs_IRF_size = 32; rf_tech.archi_Regs_FRF_size = 32; rf_tech.core_issue_width = 1;  
	    rf_tech.core_register_windows_size = 8;  rf_tech.core_number_hardware_threads = 4;	    
	    rf_tech.core_temperature=360; rf_tech.core_tech_node=65; rf_tech.core_opcode_width =8;  rf_tech.core_virtual_address_width = 64;
 	    /*io*/
	    io_tech.unit_scap=0.0; io_tech.unit_icap=0.0; io_tech.unit_lcap=0.0; io_tech.vss=0.0; io_tech.op_freq=0;
	    io_tech.i_o_style=OUT; io_tech.opt_io_buffer_num=0; io_tech.ustrip_len=0.0; io_tech.bus_width=0;	
	    io_tech.bus_size=0; io_tech.io_access_time=0; io_tech.io_cycle_time=0; io_tech.unit_ecap=0.0;
	    	    		
	    /*logic*/
	    logic_tech.unit_scap=0.0; logic_tech.unit_icap=0.0; logic_tech.unit_lcap=0.0; logic_tech.vss=0.0; 
	    logic_tech.op_freq=0; logic_tech.lgc_style=STATIC; logic_tech.num_gates=0; logic_tech.num_functions=0;
	    logic_tech.num_fan_in=0; logic_tech.num_fan_out=0; logic_tech.unit_ecap=0.0;
	    logic_tech.core_instruction_window_size = 64; logic_tech.core_issue_width = 1; logic_tech.core_number_hardware_threads = 4;
	    logic_tech.core_decode_width = 1;  logic_tech.archi_Regs_IRF_size = 32; logic_tech.archi_Regs_FRF_size = 32;	
	    logic_tech.core_temperature=360; logic_tech.core_tech_node=65;    
            /*alu*/
	    alu_tech.unit_scap=50.0; alu_tech.unit_icap=0.0; alu_tech.unit_lcap=0.0; alu_tech.vss=0.0;
	    alu_tech.op_freq=0; alu_tech.unit_ecap=0.0;	  
	    /*fpu*/
	    fpu_tech.unit_scap=350.0; fpu_tech.unit_icap=0.0; fpu_tech.unit_lcap=0.0; fpu_tech.vss=0.0;
	    fpu_tech.op_freq=0; fpu_tech.unit_ecap=0.0;       
	    /*mult*/
	    mult_tech.unit_scap=0.0; mult_tech.unit_icap=0.0; mult_tech.unit_lcap=0.0; mult_tech.vss=0.0;
	    mult_tech.op_freq=0; mult_tech.unit_ecap=0.0;
	    /*IB*/
	    ib_tech.core_instruction_length = 32; ib_tech.core_issue_width = 1; ib_tech.core_number_hardware_threads = 4;
	    ib_tech.core_instruction_buffer_size = 20; ib_tech.num_rwports = 1; ib_tech.core_temperature=360; ib_tech.core_tech_node=65;
	    ib_tech.core_virtual_address_width = 64; ib_tech.core_virtual_memory_page_size = 4096; 
	    /*IRS*/
	    irs_tech.core_number_hardware_threads = 4;  irs_tech.core_instruction_length = 32;  irs_tech.core_instruction_window_size = 64;
	    irs_tech.core_issue_width = 1;   
	    irs_tech.core_temperature=360; irs_tech.core_tech_node=65;
	    #ifdef McPAT05_H 
		perThreadState = 4; //from McPAT
	    #endif
	    /*INST_DECODER*/
	    decoder_tech.core_opcode_width = 8; decoder_tech.core_temperature=360; decoder_tech.core_tech_node=65;
	    /*BYPASS*/
	    bypass_tech.core_number_hardware_threads = 4;  bypass_tech.ALU_per_core = 3;   bypass_tech.machine_bits = 64; 
	    bypass_tech.FPU_per_core = 1; bypass_tech.core_opcode_width = 8; bypass_tech.core_virtual_address_width =64; bypass_tech.machine_bits = 64;
	    bypass_tech.core_store_buffer_size =32; bypass_tech.core_memory_ports = 1; bypass_tech.core_temperature=360; bypass_tech.core_tech_node=65;
	    /*EXEU*/
	    #ifdef McPAT05_H
	    C_EXEU = 100.0; //pF
	    #endif
	    /*PIPELINE*/
	    pipeline_tech.core_number_hardware_threads = 4;  pipeline_tech.core_fetch_width = 1; pipeline_tech.core_decode_width = 1;
	    pipeline_tech.core_issue_width = 1; pipeline_tech.core_commit_width = 1; pipeline_tech.core_instruction_length = 32;
	    pipeline_tech.core_virtual_address_width = 64;  pipeline_tech.core_opcode_width = 8; pipeline_tech.core_int_pipeline_depth = 12;
	    pipeline_tech.machine_bits = 64;  pipeline_tech.archi_Regs_IRF_size = 32; 	pipeline_tech.core_temperature=360; pipeline_tech.core_tech_node=65; 
	    /*schedulerU*/
	    #ifdef McPAT06_H 
		perThreadState = 8; //from McPAT
	    #endif   
	    /*uarch*/
	    uarch_tech.unit_scap=0.0; uarch_tech.unit_icap=0.0; uarch_tech.unit_lcap=0.0; uarch_tech.vss=0.0;
	    uarch_tech.op_freq=0; uarch_tech.unit_ecap=0.0;
	    /*btb*/
            btb_tech.unit_scap = 8192.0; btb_tech.vss = 0.0; btb_tech.op_freq = 0; 
            btb_tech.line_size = 4; btb_tech.assoc = 2; btb_tech.num_banks = 1;
	    btb_tech.throughput =1.0; btb_tech.latency = 3.0;
	    /*core--McPAT06*/
	    core_tech.core_physical_address_width=52; core_tech.core_temperature=360; core_tech.core_tech_node=65;
	    core_tech.core_virtual_address_width =64; core_tech.core_virtual_memory_page_size=4096; core_tech.core_number_hardware_threads=4;		
	    core_tech.machine_bits=64; core_tech.archi_Regs_IRF_size=32; core_tech.archi_Regs_FRF_size=32;
	    core_tech.core_issue_width=1; core_tech.core_register_windows_size=8; core_tech.core_opcode_width=8;	
	    core_tech.core_instruction_window_size=64; core_tech.core_decode_width=1; core_tech.core_instruction_length=32;
   	    core_tech.core_instruction_buffer_size=20; core_tech.ALU_per_core=3; core_tech.FPU_per_core=1; core_tech.core_ROB_size = 80;	
	    core_tech.core_store_buffer_size=32; core_tech.core_load_buffer_size=32; core_tech.core_memory_ports=1; core_tech.core_fetch_width=1;
	    core_tech.core_commit_width=1; core_tech.core_int_pipeline_depth=12; core_tech.core_phy_Regs_IRF_size=80; core_tech.core_phy_Regs_FRF_size=80; core_tech.core_RAS_size=32;	
	    core_tech.core_number_of_NoCs = 1; 	core_tech.core_number_instruction_fetch_ports = 1; core_tech.core_fp_issue_width = 1; core_tech.core_fp_instruction_window_size =64;
	    /*core--McPAT05*/
	    /*core_tech.core_physical_address_width=52; core_tech.core_temperature=360; core_tech.core_tech_node=65;
	    core_tech.core_virtual_address_width =64; core_tech.core_virtual_memory_page_size=4096; core_tech.core_number_hardware_threads=4;		
	    core_tech.machine_bits=64; core_tech.archi_Regs_IRF_size=32; core_tech.archi_Regs_FRF_size=32;
	    core_tech.core_issue_width=1; core_tech.core_register_windows_size=8; core_tech.core_opcode_width=8;	
	    core_tech.core_instruction_window_size=64; core_tech.core_decode_width=1; core_tech.core_instruction_length=32;
   	    core_tech.core_instruction_buffer_size=20; core_tech.ALU_per_core=3; core_tech.FPU_per_core=1; core_tech.core_ROB_size = 80;	
	    core_tech.core_store_buffer_size=32; core_tech.core_load_buffer_size=32; core_tech.core_memory_ports=1; core_tech.core_fetch_width=1;
	    core_tech.core_commit_width=1; core_tech.core_int_pipeline_depth=12; core_tech.core_phy_Regs_IRF_size=80; core_tech.core_phy_Regs_FRF_size=80; core_tech.core_RAS_size=32;	
	    core_tech.core_number_of_NoCs = 1;*/
	    /*cache_l2*/
	    cache_l2_tech.unit_scap = 262144.0; cache_l2_tech.vss = 0.0; cache_l2_tech.op_freq = 3500000000.0; cache_l2_tech.num_sets = 0;	    
            cache_l2_tech.line_size = 64; cache_l2_tech.num_bitlines = 0; cache_l2_tech.num_wordlines = 0; cache_l2_tech.assoc = 16;
	    cache_l2_tech.unit_icap = 0.0; cache_l2_tech.unit_lcap = 0.0; cache_l2_tech.unit_ecap = 0;
	    cache_l2_tech.num_rwports = cache_l2_tech.num_rports = cache_l2_tech.num_wports = cache_l2_tech.num_banks = 1;
	    cache_l2_tech.throughput = 2.0; cache_l2_tech.latency = 100.0; cache_l2_tech.core_physical_address_width = 52;
	    cache_l2_tech.miss_buf_size = cache_l2_tech.fill_buf_size = cache_l2_tech.prefetch_buf_size = 64; cache_l2_tech.wbb_buf_size = 64;
	    cache_l2_tech.core_virtual_address_width = cache_l2_tech.core_virtual_memory_page_size = cache_l2_tech.core_number_hardware_threads = 0;	
	    cache_l2_tech.number_entries = 0; cache_l2_tech.core_temperature=360; cache_l2_tech.core_tech_node=65; cache_l2_tech.device_type = 1; 
	    cache_l2_tech.directory_type = 1;
	    /*cache_l3*/
	    cache_l3_tech.unit_scap = 1048576.0; cache_l3_tech.vss = 0.0; cache_l3_tech.op_freq = 3500000000.0; cache_l3_tech.num_sets = 0;	    
            cache_l3_tech.line_size = 64; cache_l3_tech.num_bitlines = 0; cache_l3_tech.num_wordlines = 0; cache_l3_tech.assoc = 16;
	    cache_l3_tech.unit_icap = 0.0; cache_l3_tech.unit_lcap = 0.0; cache_l3_tech.unit_ecap = 0;
	    cache_l3_tech.num_rwports = cache_l3_tech.num_rports = cache_l3_tech.num_wports = cache_l3_tech.num_banks = 1;
	    cache_l3_tech.throughput = 2.0; cache_l3_tech.latency = 100.0; cache_l3_tech.core_physical_address_width = 52;
	    cache_l3_tech.miss_buf_size = cache_l3_tech.fill_buf_size = cache_l3_tech.prefetch_buf_size = 16; cache_l3_tech.wbb_buf_size = 16;
	    cache_l3_tech.core_virtual_address_width = cache_l3_tech.core_virtual_memory_page_size = cache_l3_tech.core_number_hardware_threads = 0;	
	    cache_l3_tech.number_entries = 0; cache_l3_tech.core_temperature=360; cache_l3_tech.core_tech_node=65; cache_l3_tech.device_type = 0;
	    cache_l3_tech.directory_type = 1;
	    /*cache_l1dir*/
	    cache_l1dir_tech.unit_scap = 1048576.0; cache_l1dir_tech.vss = 0.0; cache_l1dir_tech.op_freq = 3500000000.0; cache_l1dir_tech.num_sets = 0;	    
            cache_l1dir_tech.line_size = 16; cache_l1dir_tech.num_bitlines = 0; cache_l1dir_tech.num_wordlines = 0; cache_l1dir_tech.assoc = 16;
	    cache_l1dir_tech.unit_icap = 0.0; cache_l1dir_tech.unit_lcap = 0.0; cache_l1dir_tech.unit_ecap = 0;
	    cache_l1dir_tech.num_rwports = cache_l1dir_tech.num_rports = cache_l1dir_tech.num_wports = cache_l1dir_tech.num_banks = 1;
	    cache_l1dir_tech.throughput = 2.0; cache_l1dir_tech.latency = 100.0; cache_l1dir_tech.core_physical_address_width = 52;
	    cache_l1dir_tech.miss_buf_size = cache_l1dir_tech.fill_buf_size = cache_l1dir_tech.prefetch_buf_size = 8; cache_l1dir_tech.wbb_buf_size = 8;
	    cache_l1dir_tech.core_virtual_address_width = cache_l1dir_tech.core_virtual_memory_page_size = cache_l1dir_tech.core_number_hardware_threads = 0;	
	    cache_l1dir_tech.number_entries = 0; cache_l1dir_tech.core_temperature=360; cache_l1dir_tech.core_tech_node=65; cache_l1dir_tech.device_type = 0; 
	    cache_l1dir_tech.directory_type = 1;
	    /*cache_l2dir*/
	    cache_l2dir_tech.unit_scap = 1048576.0; cache_l2dir_tech.vss = 0.0; cache_l2dir_tech.op_freq = 3500000000.0; cache_l2dir_tech.num_sets = 0;	    
            cache_l2dir_tech.line_size = 16; cache_l2dir_tech.num_bitlines = 0; cache_l2dir_tech.num_wordlines = 0; cache_l2dir_tech.assoc = 16;
	    cache_l2dir_tech.unit_icap = 0.0; cache_l2dir_tech.unit_lcap = 0.0; cache_l2dir_tech.unit_ecap = 0;
	    cache_l2dir_tech.num_rwports = cache_l2dir_tech.num_rports = cache_l2dir_tech.num_wports = cache_l2dir_tech.num_banks = 1;
	    cache_l2dir_tech.throughput = 2.0; cache_l2dir_tech.latency = 100.0; cache_l2dir_tech.core_physical_address_width = 52;
	    cache_l2dir_tech.miss_buf_size = cache_l2dir_tech.fill_buf_size = cache_l2dir_tech.prefetch_buf_size = 8; cache_l2dir_tech.wbb_buf_size = 8;
	    cache_l2dir_tech.core_virtual_address_width = cache_l2dir_tech.core_virtual_memory_page_size = cache_l2dir_tech.core_number_hardware_threads = 0;	
	    cache_l2dir_tech.number_entries = 0; cache_l2dir_tech.core_temperature=360; cache_l2dir_tech.core_tech_node=65; cache_l2dir_tech.device_type = 0;
	    cache_l2dir_tech.directory_type = 1;
	    /*mc*/
	    mc_tech.mc_clock=400000000.0; mc_tech.llc_line_length=64; mc_tech.databus_width=128; mc_tech.addressbus_width=51; mc_tech.req_window_size_per_channel=32;
	    mc_tech.memory_channels_per_mc=2; mc_tech.IO_buffer_size_per_channel=32; 
	    mc_tech.memory_number_ranks=2; mc_tech.memory_peak_transfer_rate=6400;
	    /*router*/
	    router_tech.clockrate=3500000000.0; router_tech.has_global_link=0; router_tech.flit_bits=128; router_tech.input_buffer_entries_per_vc=16;
	    router_tech.virtual_channel_per_port=2; router_tech.input_ports=5; router_tech.horizontal_nodes=1; router_tech.vertical_nodes=2;
	    router_tech.output_ports=8; router_tech.link_throughput=1; router_tech.link_latency=1; router_tech.topology = RING; 

	    #ifdef LV2_PANALYZER_H
	    il1_pspec = NULL; il2_pspec = NULL; dl1_pspec = NULL; dl2_pspec = NULL;  itlb_pspec = NULL;
	    dtlb_pspec = NULL;  clock_pspec = NULL;  logic_pspec = NULL;  mult_pspec = NULL;	
	    bpred_pspec = NULL; rf_pspec = NULL; alu_pspec = NULL;  fpu_pspec = NULL;            
	    #endif  
	    #ifdef IO_PANALYZER_H  
	    aio_pspec = dio_pspec = NULL;
	    #endif
	    #ifdef  XML_PARSE_H_
	    p_Mp1= new ParseXML();
	    #endif /*XML_PARSE_H_*/
        }

	//Destructor
		//virtual ~Power();

	
	void setTech(ComponentId_t compID, Component::Params_t params, ptype power_type);
	void getUnitPower(ptype power_type, int user_data);
	//Pdissipation_t& getPower(Cycle_t current, ptype power_type, char *user_parms, int total_cycles);
	Pdissipation_t& getPower(Cycle_t current, ptype power_type, usagecounts_t counts, int total_cycles);
	void updatePowUsage(Pdissipation_t *comp_pusage, const I& totalPowerUsage, const I& dynamicPower, const I& leakage, const I& TDP, Cycle_t current);
	double estimateClockDieAreaSimPan();
	double estimateClockNodeCapSimPan();
	double estimateAreaMcPAT(){return p_areaMcPAT;};
	void resetCounts(usagecounts_t counts){ memset(&counts,0,sizeof(usagecounts_t));}

	// McPAT interface
	#ifdef McPAT05_H
	void McPAT05Setup();
	/*the following are no longer used*/
	void McPAT05initBasic();
	void McPATinitIcache();
	void McPATinitDcache();
	void McPATinitItlb();
	void McPATinitDtlb();
	void McPATinitIB();
	void McPATinitIRS();
	void McPATinitRF();
	void McPATinitBypass();
	void McPATinitLogic();
	void McPATinitDecoder();
	void McPATinitPipeline();
	void McPATinitClock();
	#endif

	#ifdef McPAT06_H
	void McPATSetup();
	#endif


	/*
	BOOST_SERIALIZE {
            _AR_DBG(Power,"start\n");
            ar & BOOST_SERIALIZATION_NVP(p_compID);
            ar & BOOST_SERIALIZATION_NVP(p_powerLevel);
            ar & BOOST_SERIALIZATION_NVP(p_powerMonitor);
            ar & BOOST_SERIALIZATION_NVP(p_powerModel);
	    ar & BOOST_SERIALIZATION_NVP(p_usage_cache_il1);
            _AR_DBG(Power,"done\n");
        }

        SAVE_CONSTRUCT_DATA(Power)
        {
            _AR_DBG(Power,"\n");
            ComponentId_t p_compID = t->p_compID;
            ar << BOOST_SERIALIZATION_NVP( p_compID );
        }

        LOAD_CONSTRUCT_DATA(Power)
        {
            _AR_DBG(Power,"\n");
            ComponentId_t p_compID = t->p_compID;
            ar >> BOOST_SERIALIZATION_NVP( p_compID );
            ::new(t)Power( p_compID );
        }
	*/

};
}
#endif // POWER_H




