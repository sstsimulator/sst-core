#!/usr/bin/env python
#
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
import copy

if __name__ == "__main__":

    args = copy.copy(sys.argv)
    args.pop(0)
    print(args)
    params = dict()
    for arg in args:
        if arg.startswith("--param="):
            arg = arg[8:]
            key, value = arg.split(":",1)
            params[key] = value;
    print(params)

    num_entities = int(params["num_entities"])

    link = sst.Link("left0")
    last_link = link
    for x in range(num_entities):
        comp = sst.Component("obj%d"%x, "coreTestElement.coreTestSharedObjectsComponent")
        comp.addParams(params)
        comp.addParam("myid",x)
        comp.addLink(last_link, "left", "4ns")
        if x == (num_entities - 1): last_link = link
        else: last_link = sst.Link("left{0}".format((x+1)))
        comp.addLink(last_link, "right", "4ns")
