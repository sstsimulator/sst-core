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

from sst import UnitAlgebra

one_ns     = UnitAlgebra("1ns")
two_ns     = UnitAlgebra("2ns")
four_ns    = UnitAlgebra("4ns")
eight_ns   = UnitAlgebra("8ns")
twelve_ns  = UnitAlgebra("12ns")
fifteen_ns = UnitAlgebra("15ns")

# Define the simulation components
comp_c0 = sst.Component("c1", "coreTestElement.coreTestLinks")
comp_c0.addParams({
    "id" : 0,
    "link_time_base"     : one_ns
})

comp_c1 = sst.Component("c0_1", "coreTestElement.coreTestLinks")
comp_c1.addParams({
    "id" : 1,
    "added_send_latency" : "10 ns"
})
comp_c1.addParam("link_time_base", two_ns)

comp_c2 = sst.Component("c1_0", "coreTestElement.coreTestLinks")
comp_c2.addParams({
    "id" : 2,
    "added_recv_latency" : fifteen_ns,
    "link_time_base"     : "3 ns"
})

comp_c3 = sst.Component("c1_1", "coreTestElement.coreTestLinks")
comp_c3.addParams({
    "id" : 3,
    "added_send_latency" : "20 ns",
    "added_recv_latency" : "25 ns",
    "link_time_base"     : four_ns
})

# Define the links
# Test UnitAlgebra with "connect" + non-default latency
link_0 = sst.Link("link_0")
link_0.connect( (comp_c0, "Wlink", two_ns), (comp_c0, "Wlink", two_ns) )

link_0_1 = sst.Link("link_0_1")
# Test UnitAlgebra with "addLink" + non-default latency
comp_c0.addLink(link_0_1, "Elink", four_ns)
comp_c1.addLink(link_0_1, "Wlink", four_ns)

# Test UnitAlgebra with "connect" + default latency
link_1_2 = sst.Link("link_1_1", eight_ns)
link_1_2.connect( (comp_c1, "Elink"), (comp_c2, "Wlink") )

# Test UnitAlgebra with "addLink" + default latency
link_2_3 = sst.Link("link_2_3", twelve_ns)
comp_c2.addLink(link_2_3, "Elink")
comp_c3.addLink(link_2_3, "Wlink")

# Test non-UnitAlgebra with default latency
link_3 = sst.Link("link_3", "16 ns")
link_3.connect( (comp_c3, "Elink"), (comp_c3, "Elink") )

