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

# Define SST core options
sst.setProgramOption("stop-at", "10000s")



# Define the simulation components
comp_module = sst.Component("moduleLoader", "coreTestElement.coreTestModuleLoader")
comp_module.addParams({
      "count" : "100000",
      "seed_z" : "1053",
      "verbose" : "1",
      "rng" : "marsaglia",
      "seed_w" : "1447"
})


# Define the simulation links
