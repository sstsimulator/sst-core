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


# Set up senders using anonymous subcomponents
loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")
loader0.addParam("unnamed_subcomponent", "coreTestElement.SubCompSender")
loader0.addParam("num_subcomps", "2")
loader0.addParam("sendCount", 15)
loader0.enableAllStatistics()

# Set up receivers using anonymous subcomponents
loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")
loader1.addParam("unnamed_subcomponent", "coreTestElement.SubCompReceiver")
loader1.addParam("num_subcomps", "2")
loader1.enableAllStatistics()

# Set up links
link0 = sst.Link("myLink0")
link0.connect((loader0, "port0", "5ns"), (loader1, "port0", "5ns"))
link1 = sst.Link("myLink1")
link1.connect((loader0, "port1", "5ns"), (loader1, "port1", "5ns"))

#sst.enableAllStatisticsForAllComponents()
sst.setStatisticLoadLevel(1)
