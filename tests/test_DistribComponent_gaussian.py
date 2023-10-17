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

d0 = sst.Component("d0", "coreTestElement.simpleDistribComponent")
d0.addParams({
		"distrib" : "gaussian",
		"mean" : "32",
		"stddev" : "4",
		"count" : "100000000",
		"binresults" : "1"
        })
