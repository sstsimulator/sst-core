# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
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

sub0 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSender",0)
sub0.addParam("sendCount", 15)
sub0.enableAllStatistics()


loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")

sub1 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompReceiver",0)
sub1.enableAllStatistics()

link = sst.Link("myLink1")
link.connect((sub0, "sendPort", "5ns"), (sub1, "recvPort", "5ns"))

sst.setStatisticLoadLevel(1)
