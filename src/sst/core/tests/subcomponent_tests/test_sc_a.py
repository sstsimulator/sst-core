import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")


# Set up sender using anonymous subcomponent
loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")
loader0.addParam("unnamed_subcomponent", "simpleElementExample.SubCompSender")
loader0.addParam("sendCount", 15)
loader0.enableAllStatistics()

# Set up receiver using anonymous subcomponent
loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")
loader1.addParam("unnamed_subcomponent", "simpleElementExample.SubCompReceiver")
loader1.enableAllStatistics()

# Set up link
link = sst.Link("myLink")
link.connect((loader0, "port0", "5ns"), (loader1, "port0", "5ns"))

sst.setStatisticLoadLevel(1)
