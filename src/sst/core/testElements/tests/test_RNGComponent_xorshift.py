# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Define the simulation components
comp_clocker0 = sst.Component("clocker0", "simpleElementExample.simpleRNGComponent")
comp_clocker0.addParams({
      "count" : """100000""",
      "seed" : """1447""",
      "verbose" : "1",
      "rng" : """xorshift"""
})


# Define the simulation links
# End of generated output.
