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
import sys

import sst

# Ring of components which exchange events with large payloads with their neighbors. When run on several ranks and
# threads, this tests the transfer of serialized data between ranks, including during the distribution of the
# configuration graph (which the padding parameter makes larger) and the init phase.
#
# The optional scenario argument (--model-options) selects the payloads:
#   default:     large payloads during init and the run phase
#   large_setup: much larger payloads during init and setup than during the run phase, so that the sync buffers grow
#                large before the run phase and are released after setup
num_comps = 8
scenario = sys.argv[1] if len(sys.argv) > 1 else "default"

params = {
    "num_cycles" : 20,
    "events_per_cycle" : 4,
    "max_payload" : 4096,
    "init_payload" : 1000,
    "padding" : "x" * 3000
}
if scenario == "large_setup":
    params.update({
        "events_per_cycle" : 1,
        "max_payload" : 256,
        "init_payload" : 200000,
        "setup_events" : 50,
        "setup_payload" : 50000
    })
elif scenario != "default":
    sys.exit("Unknown scenario: " + scenario)

comps = []
for i in range(num_comps):
    comp = sst.Component("c{0}".format(i), "coreTestElement.coreTestPayloadComponent")
    comp.addParams(params)
    comps.append(comp)

for i in range(num_comps):
    link = sst.Link("link_{0}".format(i))
    link.connect( (comps[i], "Elink", "1ns"), (comps[(i + 1) % num_comps], "Wlink", "1ns") )
