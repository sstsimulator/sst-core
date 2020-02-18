import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")


# Set up senders using anonymous subcomponents
loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoaderLegacy")
loader0.addParam("clock", "1.5GHz")
loader0.addParam("unnamed_subcomponent", "simpleElementExample.SubCompSenderLegacy")
loader0.addParam("num_subcomps", "2")
loader0.addParam("sendCount", 15)
loader0.enableAllStatistics()

# Set up receivers using anonymous subcomponents
loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoaderLegacy")
loader1.addParam("clock", "1.0GHz")
loader1.addParam("unnamed_subcomponent", "simpleElementExample.SubCompReceiverLegacy")
loader1.addParam("num_subcomps", "2")
loader1.enableAllStatistics()

# Set up links
link0 = sst.Link("myLink0")
link0.connect((loader0, "port0", "5ns"), (loader1, "port0", "5ns"))
link1 = sst.Link("myLink1")
link1.connect((loader0, "port1", "5ns"), (loader1, "port1", "5ns"))

#sst.enableAllStatisticsForAllComponents()
sst.setStatisticLoadLevel(1)
