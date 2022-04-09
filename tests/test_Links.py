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
import sys

dangling = False
wrong_port = False
if len(sys.argv) == 2:
    if sys.argv[1] == "dangling": dangling=True
    if sys.argv[1] == "wrong_port": wrong_port=True

# Define the simulation components
comp_c0 = sst.Component("c1", "coreTestElement.coreTestLinks")
comp_c0.addParams({
    "id" : 0,
    "link_time_base"     : "1 ns"
})

comp_c1 = sst.Component("c0_1", "coreTestElement.coreTestLinks")
comp_c1.addParams({
    "id" : 1,
    "added_send_latency" : "10 ns",
    "link_time_base"     : "2 ns"
})

comp_c2 = sst.Component("c1_0", "coreTestElement.coreTestLinks")
comp_c2.addParams({
    "id" : 2,
    "added_recv_latency" : "15 ns",
    "link_time_base"     : "3 ns"
})

comp_c3 = sst.Component("c1_1", "coreTestElement.coreTestLinks")
comp_c3.addParams({
    "id" : 3,
    "added_send_latency" : "20 ns",
    "added_recv_latency" : "25 ns",
    "link_time_base"     : "4 ns"
})

# Define the links
link_0 = sst.Link("link_0")
if not dangling:
    link_0.connect( (comp_c0, "Wlink", "2 ns"), (comp_c0, "Wlink", "2 ns") )
else:
    comp_c0.addLink(link_0, "Wlink", "2 ns")

link_0_1 = sst.Link("link_0_1")
if not wrong_port:
    link_0_1.connect( (comp_c0, "Elink", "4 ns"), (comp_c1, "Wlink", "4 ns") )
else:
    link_0_1.connect( (comp_c0, "Qlink", "4 ns"), (comp_c1, "Wlink", "4 ns") )

link_1_2 = sst.Link("link_1_1")
link_1_2.connect( (comp_c1, "Elink", "8 ns"), (comp_c2, "Wlink", "8 ns") )

link_2_3 = sst.Link("link_2_3")
link_2_3.connect( (comp_c2, "Elink", "12 ns"), (comp_c3, "Wlink", "12 ns") )

link_3 = sst.Link("link_3")
link_3.connect( (comp_c3, "Elink", "16 ns"), (comp_c3, "Elink", "16 ns") )

