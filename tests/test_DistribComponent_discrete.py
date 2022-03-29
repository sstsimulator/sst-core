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

d0 = sst.Component("d0", "coreTestElement.simpleDistribComponent")
d0.addParams({
		"distrib" : "discrete",
		"probcount" : "5",
		"prob0" : "0.1",
		"prob1" : "0.3",
		"prob2" : "0.35",
		"prob3" : "0.15",
		"prob4" : "0.1",
		"count" : "100000000",
		"binresults" : "1"
        })
