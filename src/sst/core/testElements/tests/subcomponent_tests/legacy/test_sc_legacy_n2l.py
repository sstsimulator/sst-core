import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")


# Set up senders using slot and user subcomponents
loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoaderLegacy")
loader0.addParam("clock", "1.5GHz")

sub0 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompSlotLegacy",0)
sub0.addParam("sendCount", 15)
sub0.addParam("unnamed_subcomponent", "simpleElementExample.SubCompSenderLegacy")
sub0.addParam("num_subcomps","2");
sub0.enableAllStatistics()


# Set up receivers using slot and user subcomponents
loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoaderLegacy")
loader1.addParam("clock", "1.0GHz")

sub1 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompSlotLegacy",0)
sub1.addParam("unnamed_subcomponent", "simpleElementExample.SubCompReceiverLegacy")
sub1.addParam("num_subcomps","2");
sub1.enableAllStatistics()


# Set up links
link0 = sst.Link("myLink0")
link0.connect((sub0, "slot_port0", "5ns"), (sub1, "slot_port0", "5ns"))

link1 = sst.Link("myLink1")
link1.connect((sub0, "slot_port1", "5ns"), (sub1, "slot_port1", "5ns"))


sst.setStatisticLoadLevel(1)
