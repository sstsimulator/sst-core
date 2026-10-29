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

# Checks that the handle returned by Component.addPortModule() refers to
# the port module it was returned for, when a component has several port
# modules: two on the same port, and one on another port.
#
# a sends events to b, which forwards them to c, which counts what it gets.
# b has two receive-side port modules on its "left" port and one send-side
# port module on its "right" port.  Each is configured only through its
# own handle, and each has all statistics enabled through that handle, so
# the per-module statistics show which module got which settings:
#   - b.left #0:  passes events through
#   - b.left #1:  marks events as modified ("modify" set through its handle)
#   - b.right #0: passes events through
# c should count every event as modified.
import sst

sst.setStatisticLoadLevel(4)
sst.setStatisticOutput("sst.statOutputConsole", {"outputrank": False, "outputsimtime": True})

COMP = "coreTestElement.coreTestPortModuleComponent"
PM = "coreTestElement.portmodules.test"

a = sst.Component("a", COMP)
a.addParam("sendcount", 5)
b = sst.Component("b", COMP)
c = sst.Component("c", COMP)
c.enableAllStatistics()
sst.Link("l1").connect((a, "right", "1ns"), (b, "left", "1ns"))
sst.Link("l2").connect((b, "right", "1ns"), (c, "left", "1ns"))

left0 = b.addPortModule("left", PM)
left1 = b.addPortModule("left", PM)
right0 = b.addPortModule("right", PM)

left1.addParam("modify", True)
right0.addParam("install_on_send", True)

for pm in (left0, left1, right0):
    pm.setStatisticLoadLevel(4)
    pm.enableAllStatistics()
