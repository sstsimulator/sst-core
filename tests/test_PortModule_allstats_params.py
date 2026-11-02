# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2026, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

# Checks that parameters passed to PortModule.enableAllStatistics() are
# used.  The port module's statistics ask for sst.HistogramStatistic; if
# the parameters were dropped, the default sst.AccumulatorStatistic would
# be used instead.
import sst

sst.setStatisticLoadLevel(4)
sst.setStatisticOutput("sst.statOutputConsole", {"outputrank": False, "outputsimtime": True})

COMP = "coreTestElement.coreTestPortModuleComponent"

# a sends events to b, which forwards them to c
a = sst.Component("a", COMP)
a.addParam("sendcount", 5)
b = sst.Component("b", COMP)
c = sst.Component("c", COMP)
sst.Link("l1").connect((a, "right", "1ns"), (b, "left", "1ns"))
sst.Link("l2").connect((b, "right", "1ns"), (c, "left", "1ns"))

pm = b.addPortModule("right", "coreTestElement.portmodules.test", {"install_on_send": True})
pm.setStatisticLoadLevel(4)
pm.enableAllStatistics({"type": "sst.HistogramStatistic", "minvalue": "0", "binwidth": "1", "numbins": "4"})
