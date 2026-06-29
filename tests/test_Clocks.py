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
import sst

total_components = 8
comp_list = []

# Define the simulation components
for x in range(total_components):
    comp_list.append(sst.Component("clocker{0}".format(x), "coreTestElement.coreTestClockerComponent"))

# Links
count = 0;
prior_comp = None

for x in comp_list:
    if prior_comp is None:
        prior_comp = x
        continue

    link = sst.Link("link{0}".format(count), "6ns")
    count += 1
    prior_comp.addLink(link, "right")
    x.addLink(link, "left")
    prior_comp = x
