// A cpu component that can report unit power 
// Built for testing the power model.

#ifndef _CPU_POWER_H
#define _CPU_POWER_H

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>
#include <sst/timeConverter.h>
#include "../power/power.h"

using namespace SST;

#if DBG_CPU_POWER
#define _CPU_POWER_DBG( fmt, args...)\
         printf( "%d:Cpu_power::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _CPU_POWER_DBG( fmt, args...)
#endif

class Cpu_power : public Component {
        typedef enum { WAIT, SEND } state_t;
        typedef enum { WHO_NIC, WHO_MEM } who_t;
    public:
        Cpu_power( ComponentId_t id, Params_t& params ) :
            Component( id ),
            params( params ),
            state(SEND),
            who(WHO_MEM), 
            frequency( "2.2GHz" )
        {
            _CPU_POWER_DBG( "new id=%lu\n", id );

	    registerExit();

            Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _CPU_POWER_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("clock") ) {
                    frequency = it->second;
                }    
                ++it;
            } 
            
            mem = LinkAdd( "MEM" );
            handler = new EventHandler< Cpu_power, bool, Cycle_t >
                                                ( this, &Cpu_power::clock );
            TimeConverter* tc = registerClock( frequency, handler );
	    printf("CPU_POWER period: %ld\n",tc->getFactor());
            _CPU_POWER_DBG("Done registering clock\n");

	    
        }
        int Setup() {
            // report/register power dissipation	    
	    power = new Power(Id());
            power->setTech(Id(), params, CACHE_IL1);
	    //power->setTech(Id(), params, CACHE_IL2);
	    power->setTech(Id(), params, CACHE_DL1);
	    //power->setTech(Id(), params, CACHE_DL2);
	    power->setTech(Id(), params, CACHE_ITLB);
	    power->setTech(Id(), params, CACHE_DTLB);
	    power->setTech(Id(), params, RF);
	    power->setTech(Id(), params, IB);	    
    	    /*power->setTech(Id(), params, ISSUE_Q);
	    power->setTech(Id(), params, INST_DECODER);*/
	    power->setTech(Id(), params, PIPELINE);
	    power->setTech(Id(), params, BYPASS);	    	    	    	    
	    /*power->setTech(Id(), params, LOGIC);*/
	    power->setTech(Id(), params, EXEU_ALU);
	    power->setTech(Id(), params, EXEU_FPU);
	    /*power->setTech(Id(), params, EXEU);*/
	    power->setTech(Id(), params, LSQ);
	    power->setTech(Id(), params, BPRED);
	    power->setTech(Id(), params, SCHEDULER_U);
	    power->setTech(Id(), params, RENAME_U);
	    /*power->setTech(Id(), params, RAT);
	    power->setTech(Id(), params, ROB);*/
	    power->setTech(Id(), params, BTB);
	    power->setTech(Id(), params, LOAD_Q);
	    power->setTech(Id(), params, CACHE_L1DIR);
	    power->setTech(Id(), params, CACHE_L2DIR);
	    power->setTech(Id(), params, CACHE_L2);
	    power->setTech(Id(), params, CACHE_L3);
	    power->setTech(Id(), params, MEM_CTRL);
	    power->setTech(Id(), params, ROUTER);
	    //power->setTech(Id(), params, CLOCK); //clock should be the last in McPAT
	    //power->setTech(Id(), params, IO);
           return 0;
        }
        int Finish() {
	    pstats = readPowerStats(this);
	    using namespace io_interval; std::cout <<"ID " << Id() <<": current total power = " << pstats.currentPower << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": leakage power = " << pstats.leakagePower << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": runtime power = " << pstats.runtimeDynamicPower << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": TDP = " << pstats.TDP << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": total energy = " << pstats.totalEnergy << " J" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": peak power = " << pstats.peak << " W" << std::endl;
	    using namespace io_interval; std::cout <<"ID " << Id() <<": current cycle = " << pstats.currentCycle << std::endl;
            _CPU_POWER_DBG("\n");
	    unregisterExit();
            return 0;
        }


    private:

        Cpu_power( const Cpu_power& c );

        bool clock( Cycle_t );
        ClockHandler_t* handler;
        bool handler1( Time_t time, Event *e );

        Params_t    params;
        Link*       mem;
        state_t     state;
        who_t       who;
	std::string frequency;

	Pdissipation_t pdata, pstats;
	Power *power;
	usagecounts_t mycounts;  //over-specified struct that holds usage counts of its sub-components


#if WANT_CHECKPOINT_SUPPORT2	
        BOOST_SERIALIZE {
	    printf("cpu_power::serialize()\n");
            _AR_DBG( Cpu_power, "start\n" );
	    printf("  doing void cast\n");
            BOOST_VOID_CAST_REGISTER( Cpu_power*, Component* );
	    printf("  base serializing: component\n");
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Component );
	    printf("  serializing: mem\n");
            ar & BOOST_SERIALIZATION_NVP( mem );
	    printf("  serializing: handler\n");
            ar & BOOST_SERIALIZATION_NVP( handler );
            _AR_DBG( Cpu_power, "done\n" );
        }


/*
        SAVE_CONSTRUCT_DATA( Cpu_power ) {
            _AR_DBG( Cpu_power, "\n" );

            ComponentId_t   id     = t->_id;
            Clock*          clock  = t->_clock;
            Params_t        params = t->params;

            ar << BOOST_SERIALIZATION_NVP( id );
            ar << BOOST_SERIALIZATION_NVP( clock );
            ar << BOOST_SERIALIZATION_NVP( params );
        } 
        LOAD_CONSTRUCT_DATA( Cpu_power ) {
            _AR_DBG( Cpu_power, "\n" );

            ComponentId_t   id;
            Clock*          clock;
            Params_t        params;

            ar >> BOOST_SERIALIZATION_NVP( id );
            ar >> BOOST_SERIALIZATION_NVP( clock );
            ar >> BOOST_SERIALIZATION_NVP( params );

            ::new(t)Cpu_power( id, clock, params );
        } 
*/
#endif
};

#endif
