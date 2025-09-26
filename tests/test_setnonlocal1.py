# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
import sst

# Define SST Program Options:
sst.setProgramOption("partitioner", "sst.self")

# Set up receiver using user subcomponent
loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("verbose", 1)
loader1.setRank(1,0)

sub1 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompReceiver",0)
sub1.addParam("verbose", 1)

sub1.enableAllStatistics()

# Define the SST Links
link0 = sst.Link("link0")

# Add link to subcomponent first, then set as non-local
sub1.addLink(link0, "recvPort", "1 us")
link0.setNonLocal(0, 0)

# Define SST Statistics Options:
sst.setStatisticLoadLevel(1)
sst.setStatisticOutput("sst.statOutputConsole")

