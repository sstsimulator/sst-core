import sst

# Define the simulation components
comp_count = 4   # Number of components
start = 0        # This component should start the exchange
comps = []

for x in range(0,comp_count):
    comp = sst.Component("c" + str(x), "coreTestElement.coreTestCheckpoint")
    comp.addParams({
      "clock_frequency" : "100 kHz",
      "clock_duty_cycle" : 15,
      "test_string" : "hello",
      "output_prefix" : "c" + str(x) + " talking",
      "output_verbose" : 2,
      })

    if x == start:
        comp.addParam("starter", True)
        #comp.addParam("counter", 1000000)
        #comp.addParam("counter", 1000000000)
        comp.addParam("counter", 1000)
        comp.addParam("clock_duty_cycle", 20)
        comp.addParam("dist_discrete_probs", [0.1, 0.3, 0.1, 0.2, 0.3])
    else:
        comp.addParam("starter", False)
    comps.append(comp)

for x in range(0,comp_count):
    next_x = (x+1) % comp_count
    link = sst.Link("link" + str(x))
    link.connect( ( comps[x], "port_right", "1us"), (comps[next_x], "port_left", "1us") )

# Stats config
sst.setStatisticLoadLevel(3)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.setStatisticOutput("sst.statOutputHDF5")
#sst.setStatisticOutput("sst.statOutputJSON", {"outputsimtime" : "1"})
#sst.setStatisticOutput("sst.statOutputTxt")
#sst.setStatisticOutput("sst.statOutputCSV", {
#    "filepath" : "test_Checkpoint_stats.csv",
#    "separator" : "; " })
sst.enableStatisticForComponentName("c0", "rngvals",
    {"type":"sst.HistogramStatistic",
     "minvalue" : 0,
     "binwidth" : 400000000,
     "numbins" : 10,
     "IncludeOutOfBounds" : "T"
})
#sst.enableAllStatisticsForAllComponents()
sst.enableAllStatisticsForAllComponents({ "rate" : "100us" })

# Ensure that this will work for all parallelism.  The only case we
# need to worry about is if comp_count <= num_threads and we have more
# than one rank.
num_threads = sst.getThreadCount()
num_ranks = sst.getMPIRankCount()
if num_threads >= comp_count and num_ranks > 1:
    sst.setProgramOption("partitioner","self")
    for x,comp in enumerate(comps):
        comp.setRank(x % num_ranks, x // num_ranks)

