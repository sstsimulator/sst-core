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

# Define SST core options
sst.setProgramOption("stop-at", "10us")

loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")

sub0_0 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSender",0)
sub0_0.addParam("sendCount", 15)
sub0_0.enableAllStatistics()

sub0_1 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompReceiver",1)
sub0_1.enableAllStatistics()

loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")

sub1_0 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompReceiver",0)
sub1_0.enableAllStatistics()
sub1_1 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompSender",1)
sub1_1.addParam("sendCount", 15)
sub1_1.enableAllStatistics()

link = sst.Link("myLink1")
link.connect((sub0_0, "sendPort", "5ns"), (sub1_0, "recvPort", "5ns"))

link = sst.Link("myLink2")
link.connect((sub1_1, "sendPort", "5ns"), (sub0_1, "recvPort", "5ns"))

sst.setStatisticLoadLevel(1)
