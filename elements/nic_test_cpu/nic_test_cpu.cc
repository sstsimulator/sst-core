#include <sst_config.h>

#include "nic_test_cpu.h"


int nic_test_cpu_debug; 


bool
// nic_test_cpu::handle_nic_events(Time_t time, Event *event)
nic_test_cpu::handle_nic_events(Event *event)
{
//     _NIC_TEST_CPU_DBG(2, "CPU %lu (rank %d) got an event from the local NIC at time %.9f\n",Id(), my_rank, time);
    _NIC_TEST_CPU_DBG(2, "CPU %lu (rank %d) got an event from the local NIC at time FIXME\n",Id(), my_rank);

    return false;
}


extern "C" {
nic_test_cpu *
nic_test_cpuAllocComponent(SST::ComponentId_t id, 
                           SST::Component::Params_t &params)
{
    return new nic_test_cpu(id, params);
}
}

#if WANT_CHECKPOINT_SUPPORT
BOOST_CLASS_EXPORT(nic_test_cpu)

// BOOST_CLASS_EXPORT_TEMPLATE4(SST::EventHandler,
//     nic_test_cpu, bool, SST::Time_t, SST::Event *)

BOOST_CLASS_EXPORT_TEMPLATE3(SST::EventHandler,
    nic_test_cpu, bool, SST::Event *)
#endif
