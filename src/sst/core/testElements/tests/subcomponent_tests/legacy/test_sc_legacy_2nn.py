import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")

# Set up senders using slots and user subcomponent
loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoaderLegacy")
loader0.addParam("clock", "1.5GHz")
loader0.enableAllStatistics()

sub0_0 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompSlotLegacy",0)

sub0_0_0 = sub0_0.setSubComponent("mySubCompSlot","simpleElementExample.SubCompSenderLegacy",0);
sub0_0_0.addParam("sendCount", 15)
sub0_0_0.enableAllStatistics()


sub0_1 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompSlotLegacy",1)

sub0_1_0 = sub0_1.setSubComponent("mySubCompSlot","simpleElementExample.SubCompSenderLegacy",0);
sub0_1_0.addParam("sendCount", 15)
sub0_1_0.enableAllStatistics()


# Set up receivers using slots and user subcomponent
loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoaderLegacy")
loader1.addParam("clock", "1.0GHz")

sub1_0 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompSlotLegacy",0)

sub1_0_0 = sub1_0.setSubComponent("mySubCompSlot", "simpleElementExample.SubCompReceiverLegacy",0)
sub1_0_0.enableAllStatistics()

sub1_1 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompSlotLegacy",1)

sub1_1_0 = sub1_1.setSubComponent("mySubCompSlot", "simpleElementExample.SubCompReceiverLegacy",0)
sub1_1_0.enableAllStatistics()


# Set up links
link0 = sst.Link("myLink0")
link0.connect((sub0_0_0, "sendPort", "5ns"), (sub1_0_0, "recvPort", "5ns"))

link1 = sst.Link("myLink1")
link1.connect((sub0_1_0, "sendPort", "5ns"), (sub1_1_0, "recvPort", "5ns"))


sst.setStatisticLoadLevel(1)
