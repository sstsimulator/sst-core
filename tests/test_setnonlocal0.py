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

# Set up sender using user subcomponent
loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoader")
loader0.addParams({
     "clock" : "0.1GHz",
     "verbose" : "1"
})
loader0.setRank(0,0)

sub0 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSender",0)
sub0.addParams({
     "verbose" : "1",
     "sendCount" : "15"
})

sub0.enableAllStatistics()

# Define the SST Links
link0 = sst.Link("link0")

# Set non-local first, then add link to subcomponent
link0.setNonLocal(1,0)
sub0.addLink(link0, "sendPort", "1 us")

# Define SST Statistics Options:
sst.setStatisticLoadLevel(1)
sst.setStatisticOutput("sst.statOutputConsole")

