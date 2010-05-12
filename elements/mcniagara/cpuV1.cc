#include "cpu.h"

#include <sst/memEvent.h>

// bool Cpu::clock( Cycle_t current, Time_t epoch )
bool Cpu::clock( Cycle_t current )
{
    static unsigned long num_calls = 0;
    unsigned long i;
    num_calls++;
    _CPU_DBG("id=%lu cycle=%lu getCurrentSimTime=%lu num_calls=%d this=%p\n",
             Id(), current, getCurrentSimTime(), num_calls, this);
    for (i = cyclesAtLastClock; i < getCurrentSimTime(); i++)
       mcCpu->sim_cycle(i);
    cyclesAtLastClock = getCurrentSimTime();
    return false;
    
/***
    //_CPU_DBG("id=%lu getCurrentSimTime=%lu \n", Id(), current );
//     printf("In CPU::clock\n");
    MemEvent* event = NULL; 

    if (current == 100 ) unregisterExit();

    if ( state == SEND ) { 
        if ( ! event ) event = new MemEvent();

        if ( who == WHO_MEM ) { 
            event->address = 0x1000; 
            who = WHO_NIC;
        } else {
            event->address = 0x10000000; 
            who = WHO_MEM;
        }

        _CPU_DBG("xxx: send a MEM event address=%#lx @ cycle %ld\n", event->address, current );
// 	mem->Send( 3 * epoch, event );
	mem->Send( (Cycle_t)3, event );
// 	printf("CPU::clock -> setting state to WAIT\n");
        state = WAIT;
    } else {
// 	printf("Entering state WAIT\n");
        if ( ( event = static_cast< MemEvent* >( mem->Recv() ) ) ) {
// 	    printf("Got a mem event\n");
	  _CPU_DBG("xxx: got a MEM event address=%#lx @ cycle %ld\n", event->address, current );
// 	  printf("CPU::clock -> setting state to SEND\n");
            state = SEND;
        }
    }
    return false;
***/
}

bool Cpu::memEvent( Event* event  )
{
     MemEvent *mevent;
     mevent = dynamic_cast<MemEvent*>(event);
     if (mevent)
          _CPU_DBG( "id=%lu cycle=%lu addr=%lx\n", Id(),
                getCurrentSimTime(), mevent->address );
     else
          _CPU_DBG( "id=%lu cycle=%lu\n", Id(),
                getCurrentSimTime() );
    //cpu->Send( 3 * (1.0/frequency), static_cast<CompEvent*>(event) );
    delete event; // SHOULD I BE DOING THIS?? it seems to work...
    return false;
}

void Cpu::memoryAccess(OffCpuIF::access_mode mode,
                       long long unsigned int address,
                       long unsigned int data_size)
{
     MemEvent *event;
    _CPU_DBG( "id=%lu cycle=%lu\n", Id(),
              getCurrentSimTime() );
     // who deletes this event? Should we reuse it?
     event = new MemEvent();
     if (mode == AM_READ)
        event->type = MemEvent::MEM_LOAD;
     else
        event->type = MemEvent::MEM_STORE;
     event->address = address + Id();
     //memLink->Send(getCurrentSimTime(), event);
     delete event;  // remove this if sending
}

void Cpu::NICAccess(OffCpuIF::access_mode mode, long unsigned int data_size)
{
    _CPU_DBG( "id=%lu cycle=%lu\n", Id(), getCurrentSimTime() );
}


extern "C" {
Cpu* mcniagaraAllocComponent( SST::ComponentId_t id, SST::Simulation* sim,
                                    SST::Component::Params_t& params )
{
//     printf("cpuAllocComponent--> sim = %p\n",sim);
    return new Cpu( id, sim, params );
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(Cpu)

// BOOST_CLASS_EXPORT_TEMPLATE4( SST::EventHandler,
//                                 Cpu, bool, SST::Cycle_t, SST::Time_t )
BOOST_CLASS_EXPORT_TEMPLATE3( SST::EventHandler,
                                Cpu, bool, SST::Cycle_t)
#endif
