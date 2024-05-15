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

def sstcreatemodel():

    sst.creategraph()

    if sst.verbose():
	print "SST Random Number Generation Test Component Model"

    id = sst.createcomponent("rng0", "coreTestElement.simpleRNGComponent")
    sst.addcompparam(id, "count", "1000")
    sst.addcompparam(id, "seed_w", "1447")
    sst.addcompparam(id, "seed_z", "1053")

    return 0
