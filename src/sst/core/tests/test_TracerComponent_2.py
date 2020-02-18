# Simple SST python tests to demonstrate use of simpleTrace.
# This test generates a trace of all memory references going to memController
# Generated Files are -trace: test-2_memory_references_trace.txt,
# stats: test-2_memory_references_stats.txt

## arch model
#
#  comp_cpu <-> comp_l1cache <-> comp_l2cache <-> comp_tracer <-> comp_memory
#
##

import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "1ms")

#define simulation components
comp_cpu = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu.addParams({
    "num_loadstore"  : "100",
    "commFreq"       : "100",
    "memSize"        : "0x10000",
    "do_write"       : "1",
    "workPerCycle"   : "1000",
})

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
    "access_latency_cycles" : "5",
    "cache_frequency"       : "2 Ghz",
    "replacement_policy"    : "lru",
    "coherence_protocol"    : "MSI",
    "associativity"         : "4",
    "cache_line_size"       : "64",
    "debug_level"           : "8",
    "L1"                    : "1",
    "debug"                 : "0",
    "cache_size"            : "4 KB",
})

comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
    "access_latency_cycles" : "20",
    "cache_frequency"       : "2 Ghz",
    "replacement_policy"    : "lru",
    "coherence_protocol"    : "MSI",
    "associativity"         : "4",
    "cache_line_size"       : "64",
    "debug_level"           : "8",
    "L1"                    : "0",
    "debug"                 : "0",
    "cache_size"            : "64 KB",
})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
    "clock"                 : "2 Ghz",
    "request_width"         : "64",
    "debug"                 : "0",
})
backend = comp_memory.setSubComponent("backend", "memHierarchy.simpleMem")
backend.addParams({
    "mem_size"              : "1024MiB",
})

comp_tracer = sst.Component("tracer", "coreTestElement.simpleTracerComponent")
comp_tracer.addParams({
    "clock"      : "2 Ghz",
    "debug"      : "8",
    "statistics" : "1",
    "pageSize"   : "4096",
    "accessLatencyBins" : "10",
    "tracePrefix" : "test_simpleTracerComponent_2_mem_ref_trace.txt",
    "statsPrefix" : "test_simpleTracerComponent_2_mem_ref_stats.txt",
 })

# define the simulation links
link_cpu_l1cache = sst.Link("link_cpu_l1cache")
link_cpu_l1cache.connect((comp_cpu, "mem_link", "100ps"),(comp_l1cache, "high_network_0", "100ps"))

link_l1cache_l2cache = sst.Link("link_l1cache_l2cache")
link_l1cache_l2cache.connect((comp_l1cache, "low_network_0", "100ps"), (comp_l2cache, "high_network_0", "100ps"))

link_l2cache_tracer = sst.Link("link_l2cache_tracer")
link_l2cache_tracer.connect((comp_l2cache, "low_network_0", "100ps"), (comp_tracer, "northBus", "100ps"))

link_tracer_mem = sst.Link("link_tracer_mem")
link_tracer_mem.connect((comp_tracer, "southBus", "100ps"), (comp_memory, "direct_link", "100ps"))

