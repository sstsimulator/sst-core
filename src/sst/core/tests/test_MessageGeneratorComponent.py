# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Define the simulation components
comp_msgGen0 = sst.Component("msgGen0", "simpleElementExample.simpleMessageGeneratorComponent")
comp_msgGen0.addParams({
      "outputinfo" : """0""",
      "sendcount" : """100000""",
      "clock" : """1MHz"""
})
comp_msgGen1 = sst.Component("msgGen1", "simpleElementExample.simpleMessageGeneratorComponent")
comp_msgGen1.addParams({
      "outputinfo" : """0""",
      "sendcount" : """100000""",
      "clock" : """1MHz"""
})


# Define the simulation links
link_s_0_1 = sst.Link("link_s_0_1")
link_s_0_1.connect( (comp_msgGen0, "remoteComponent", "1000000ps"), (comp_msgGen1, "remoteComponent", "1000000ps") )
# End of generated output.
