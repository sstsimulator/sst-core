import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")


# Set up sender using slot and anonymous subcomponent
loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")

sub0 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompSlot",0)
sub0.addParam("sendCount", 15)
sub0.addParam("unnamed_subcomponent", "simpleElementExample.SubCompSender")
sub0.enableAllStatistics()


# Set up receiver using slot andanonymous subcomponent
loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")

sub1 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompSlot",0)
sub1.addParam("unnamed_subcomponent", "simpleElementExample.SubCompReceiver")
sub1.enableAllStatistics()


# Set up links
link = sst.Link("myLink1")
link.connect((sub0, "slot_port0", "5ns"), (sub1, "slot_port0", "5ns"))


sst.setStatisticLoadLevel(1)
