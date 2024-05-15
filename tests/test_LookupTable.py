# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
import sst
import inspect, os, sys


params = dict({
    # Set filename to the name of this file
    "filename" : inspect.getfile(inspect.currentframe())
    })

for i in range(10):
    comp = sst.Component("Table Comp %d"%i, "coreTestElement.simpleLookupTableComponent")
    comp.addParams(params)

