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
import sys

sst.setProgramOption("stop-at", "1us");

test = sys.argv[1]

comp = sst.Component("Component0", "coreTestElement.coreTestSerialization")
comp.addParam("test", test)
