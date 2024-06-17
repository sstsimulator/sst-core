import sst

# Define the simulation components
comp_c0 = sst.Component("c0", "coreTestElement.coreTestCheckpoint")
comp_c0.addParams({
      "starter" : 'T',
      "counter" : 1000,
      "clock_frequency" : "100 kHz",
      "clock_duty_cycle" : 20,
      "test_string" : "hi",
      "dist_discrete_probs" : [0.1, 0.3, 0.1, 0.2, 0.3],
      "output_prefix" : "c0 talking",
      "output_verbose" : 1,
})

comp_c1 = sst.Component("c1", "coreTestElement.coreTestCheckpoint")
comp_c1.addParams({
      "starter" : 'F',
      "clock_frequency" : "100 kHz",
      "clock_duty_cycle" : 15,
      "test_string" : "hello",
      "output_prefix" : "c1 talking",
      "output_verbose" : 2,
})

# Connect the components
link = sst.Link("link")
link.connect( (comp_c0, "port", "1us"), (comp_c1, "port", "1us") )

# Stats config
sst.setStatisticLoadLevel(3)
sst.setStatisticOutput("sst.statOutputConsole")
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
sst.enableAllStatisticsForAllComponents({ "rate" : "100us" })
