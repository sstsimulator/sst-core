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

########################################################
# Test Cases
########################################################
# Loading user-subcomponents into components
# Implicitly shared statistics between subcomponents
# Explicitly shared statistics between subcomponents 
########################################################

# Define SST core options
sst.setProgramOption("partitioner", "self")

verbose = 0
if len(sys.argv) > 1:
    verbose = int(sys.argv[1])

# Set up senders using user subcomponents
loader0 = sst.Component("Loader0", "coreTestElement.SubComponentLoader")
loader0.addParam("clock", "0.1GHz")
loader0.addParam("verbose", verbose)
loader0.enableAllStatistics()

sub0_0 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSender",0)
sub0_0.addParam("sendCount", 15)
sub0_0.addParam("verbose", verbose)
sub0_0.enableAllStatistics()

sub0_1 = loader0.setSubComponent("mySubComp", "coreTestElement.SubCompSender",1)
sub0_1.addParam("sendCount", 15)
sub0_1.addParam("verbose", verbose)
sub0_1.enableAllStatistics()

# Set up receivers using user subcomponents
loader1 = sst.Component("Loader1", "coreTestElement.SubComponentLoader")
loader1.addParam("verbose", verbose)

sub1_0 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompReceiver",0)
sub1_0.addParam("verbose", verbose)
sub1_0.enableAllStatistics()

sub1_1 = loader1.setSubComponent("mySubComp", "coreTestElement.SubCompReceiver",1)
sub1_1.addParam("verbose", verbose)
sub1_1.enableAllStatistics()

stat_recv = loader1.createStatistic("totalRecv", {})
sub1_0.setStatistic("numRecv", stat_recv)
sub1_1.setStatistic("numRecv", stat_recv)



# Set up links
link0 = sst.Link("myLink0")
link0.connect((sub0_0, "sendPort", "1us"), (sub1_0, "recvPort", "1us"))

link1 = sst.Link("myLink1")
link1.connect((sub0_1, "sendPort", "1us"), (sub1_1, "recvPort", "1us"))

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
