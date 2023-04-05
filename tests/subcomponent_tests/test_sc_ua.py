# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
import sst

# Define SST core options
sst.setProgramOption("stop-at", "10us")


# Set up sender using slot and anonymous subcomponent
loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")

sub0 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSlot",0)
sub0.addParam("sendCount", 15)
sub0.addParam("unnamed_subcomponent", "coreTestElement.SubCompSender")
sub0.enableAllStatistics()


# Set up receiver using slot andanonymous subcomponent
loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")

sub1 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompSlot",0)
sub1.addParam("unnamed_subcomponent", "coreTestElement.SubCompReceiver")
sub1.enableAllStatistics()


# Set up links
link = sst.Link("myLink1")
link.connect((sub0, "slot_port0", "5ns"), (sub1, "slot_port0", "5ns"))


sst.setStatisticLoadLevel(1)
