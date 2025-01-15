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

sst.setProgramOption("stop-at", "1us");

ranks = sst.getMPIRankCount();
threads = sst.getThreadCount();

test = sys.argv[1]

num_components = ranks * threads

for x in range(num_components):
    comp = sst.Component("Component{0}".format(x), "coreTestElement.coreTestOutput")
    comp.addParam("test", test)

