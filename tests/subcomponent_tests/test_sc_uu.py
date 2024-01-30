# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
import sst
import sys

# Define SST core options
sst.setProgramOption("stop-at", "10us")

verbose = 0
if len(sys.argv) > 1:
    verbose = int(sys.argv[1])

# Set up sender using slot and user subcomponent
loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")
loader0.addParam("verbose", verbose)

sub0 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSlot",0)

sub0_0 = sub0.setSubComponent("mySubCompSlot","coreTestElement.SubCompSender",0);
sub0_0.addParam("sendCount", 15)
sub0_0.addParam("verbose", verbose)
sub0_0.enableAllStatistics()


# Set up receiver using slot and user subcomponent
loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")
loader1.addParam("verbose", verbose)

sub1 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompSlot",0)

sub1_0 = sub1.setSubComponent("mySubCompSlot", "coreTestElement.SubCompReceiver",0);
sub1_0.addParam("verbose", verbose)
sub1_0.enableAllStatistics()

# Set up link
link = sst.Link("myLink")
link.connect((sub0_0, "sendPort", "5ns"), (sub1_0, "recvPort", "5ns"))


sst.setStatisticLoadLevel(1)
