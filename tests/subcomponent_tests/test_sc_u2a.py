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

# Define SST core options
sst.setProgramOption("partitioner","self")

verbose = 0
if len(sys.argv) > 1:
    verbose = int(sys.argv[1])

# Set up senders using slot and user subcomponents
loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoader")
loader0.addParam("clock", "0.1GHz")
loader0.addParam("verbose", verbose)

sub0 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSlot",0)
sub0.addParam("sendCount", 15)
sub0.addParam("unnamed_subcomponent", "coreTestElement.SubCompSender")
sub0.addParam("num_subcomps","2");
sub0.addParam("verbose", verbose)
sub0.enableAllStatistics()


# Set up receivers using slot and user subcomponents
loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("verbose", verbose)

sub1 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompSlot",0)
sub1.addParam("unnamed_subcomponent", "coreTestElement.SubCompReceiver")
sub1.addParam("num_subcomps","2");
sub1.addParam("verbose", verbose)
sub1.enableAllStatistics()


# Set up links
link0 = sst.Link("myLink0")
link0.connect((sub0, "slot_port0", "1us"), (sub1, "slot_port0", "1us"))

link1 = sst.Link("myLink1")
link1.connect((sub0, "slot_port1", "1us"), (sub1, "slot_port1", "1us"))


# Do the paritioning
num_ranks = sst.getMPIRankCount()
num_threads = sst.getThreadCount()

loader0.setRank(0,0)
if num_ranks >= 2:
    loader1.setRank(1,0)
elif num_threads > 1:
    loader1.setRank(0,1)
else:
    loader1.setRank(0,0)

sst.setStatisticLoadLevel(1)
