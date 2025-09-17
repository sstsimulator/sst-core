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
import sys

num_links = 1
if len(sys.argv) == 2:
    num_links = int(sys.argv[1])
    

sst.buildOverheadMeasureTest(2,num_links)

"""
# Define the simulation components
comp0 = sst.Component("comp0", "coreTestElement.overhead_measure")
comp0.addParams({
    "id" : 0
})

comp1 = sst.Component("comp1", "coreTestElement.overhead_measure")
comp1.addParams({
    "id" : 1
})

# Define the links

for x in range(num_links):
    link = sst.Link("link{}".format(x));
    link.connect( (comp0, "right_{}".format(x), "1 ns"), (comp1, "left_{}".format(x), "1 ns") )
"""
