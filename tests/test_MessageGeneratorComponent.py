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
sst.setProgramOption("stop-at", "10000s")

# Define the simulation components
comp_msgGen0 = sst.Component("msgGen0", "coreTestElement.simpleMessageGeneratorComponent")
comp_msgGen0.addParams({
      "outputinfo" : "0",
      "sendcount" : "100000",
      "clock" : "1MHz"
})
comp_msgGen1 = sst.Component("msgGen1", "coreTestElement.simpleMessageGeneratorComponent")
comp_msgGen1.addParams({
      "outputinfo" : "0",
      "sendcount" : "100000",
      "clock" : "1MHz"
})


# Define the simulation links
link_s_0_1 = sst.Link("link_s_0_1")
link_s_0_1.connect( (comp_msgGen0, "remoteComponent", "1000000ps"), (comp_msgGen1, "remoteComponent", "1000000ps") )
