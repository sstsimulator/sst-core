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
import inspect, os, sys

nitems = 10

params = dict({
    "num_entities" : nitems
    })

for i in range(nitems):
    comp = sst.Component("Table Comp %d"%i, "coreTestElement.simpleLookupTableComponent")
    comp.addParams(params)
    comp.addParam("myid", i)

