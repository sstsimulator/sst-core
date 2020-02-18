import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")

# Set up sender using user subcomponent
loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")

sub0 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompSender",0)
sub0.addParam("sendCount", 15)
sub0.enableAllStatistics()

# Set up receiver using user subcomponent
loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")

sub1 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompReceiver",0)
sub1.enableAllStatistics()

# Set up link
link0 = sst.Link("myLink0")
link0.connect((sub0, "sendPort", "5ns"), (sub1, "recvPort", "5ns"))

sst.setStatisticLoadLevel(1)
