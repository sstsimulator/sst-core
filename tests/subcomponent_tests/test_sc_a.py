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
sst.setProgramOption("stopAtCycle", "10us")


# Set up sender using anonymous subcomponent
loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")
loader0.addParam("unnamed_subcomponent", "coreTestElement.SubCompSender")
loader0.addParam("sendCount", 15)
loader0.enableAllStatistics()

# Set up receiver using anonymous subcomponent
loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")
loader1.addParam("unnamed_subcomponent", "coreTestElement.SubCompReceiver")
loader1.enableAllStatistics()

# Set up link
link = sst.Link("myLink")
link.connect((loader0, "port0", "5ns"), (loader1, "port0", "5ns"))

sst.setStatisticLoadLevel(1)
