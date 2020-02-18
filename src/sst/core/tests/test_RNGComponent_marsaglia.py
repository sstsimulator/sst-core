# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Define the simulation components
comp_clocker0 = sst.Component("clocker0", "simpleElementExample.simpleRNGComponent")
comp_clocker0.addParams({
      "count" : "100000",
      "seed_z" : "1053",
      "verbose" : "1",
      "rng" : "marsaglia",
      "seed_w" : "1447"
})


# Define the simulation links
# End of generated output.
