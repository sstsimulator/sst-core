import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")


# Set up sender using named subcomponent
loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoaderLegacy")
loader0.addParam("clock", "1.5GHz")
loader0.addParam("use_legacy", "true")

sub0 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSenderLegacy",0)
sub0.addParam("sendCount", 15)
sub0.enableAllStatistics()


# Set up receiver using named subcomponent
loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoaderLegacy")
loader1.addParam("clock", "1.0GHz")
loader1.addParam("use_legacy", "true")

sub1 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompReceiverLegacy",0)
sub1.enableAllStatistics()


# Set up links
link = sst.Link("myLink")
link.connect((sub0, "sendPort", "5ns"), (sub1, "recvPort", "5ns"))

sst.setStatisticLoadLevel(1)
