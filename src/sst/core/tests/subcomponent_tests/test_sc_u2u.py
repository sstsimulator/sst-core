import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")

# Set up sender using slot and user subcomponents
loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")
loader0.enableAllStatistics()

sub0 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompSlot",0)

sub0_0 = sub0.setSubComponent("mySubCompSlot","simpleElementExample.SubCompSender",0);
sub0_0.addParam("sendCount", 15)
sub0_0.enableAllStatistics()

sub0_1 = sub0.setSubComponent("mySubCompSlot","simpleElementExample.SubCompSender",1);
sub0_1.addParam("sendCount", 15)
sub0_1.enableAllStatistics()


# Set up receiver using slot and user subcomponent
loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")

sub1 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompSlot",0)

sub1_0 = sub1.setSubComponent("mySubCompSlot", "simpleElementExample.SubCompReceiver",0)
sub1_0.enableAllStatistics()

sub1_1 = sub1.setSubComponent("mySubCompSlot", "simpleElementExample.SubCompReceiver",1)
sub1_1.enableAllStatistics()


# Set up links
link0 = sst.Link("myLink0")
link0.connect((sub0_0, "sendPort", "5ns"), (sub1_0, "recvPort", "5ns"))

link1 = sst.Link("myLink1")
link1.connect((sub0_1, "sendPort", "5ns"), (sub1_1, "recvPort", "5ns"))

sst.setStatisticLoadLevel(1)
