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

########################################################################
# This script tests the basic behavior of setting general parameters
# on statistics (startat, stopat, rate)

# StatGlobal0 and StatGlobal1 components test the following:
# - sst.enableAllStatisticsForAllComponents

# StatBasic0 Component tests the following:
# - Enabling stats directly on the component object with periodic
#   writes
#    - stopat, startat, rate (periodic), resetOnOutput

# StatBasic1 Component tests the following:
# - Enabling stats directly on the component object with event-driven
#   writes
#    - stopat, startat, rate (events), resetOnOutput

# StatBasic2 Component tests the following:
# - Enabling multiple stats directly on the component object with
#   periodic writes
#    - stopat, startat, rate (periodic), resetOnOutput

# StatBasic3 Component tests the following:
# - Enabling multiple stats directly on the component object with
#   event-driven writes
#    - stopat, startat, rate (events), resetOnOutput

# StatBasic4 Component tests the following:
# - Enabling stats using the global api for setting stats on named
#   component with periodic writes
#    - stopat, startat, rate (periodic), resetOnOutput

# StatBasic5 Component tests the following:
# - Enabling stats using the global api for setting stats on named
#   component with event-driven writes
#    - stopat, startat, rate (events), resetOnOutput

# StatBasic6 Component tests the following:
# - Enabling multiple stats using the global api for setting stats on
#   named component with periodic writes
#    - stopat, startat, rate (periodic), resetOnOutput

# StatBasic7 Component tests the following:
# - Enabling multiple stats using the global api for setting stats on
#   named component with event-driven writes
#    - stopat, startat, rate (events), resetOnOutput

# StatBasic8 Component tests the following:
# - Enabling all stats directly on the component object with periodic
#   writes
#    - rate (periodic), global load-level

# StatBasic9 Component tests the following:
# - Enabling all stats directly on the component object with periodic
#   writes
#    - rate (periodic), local load-level

# StatBasic10 Component tests the following:
# - Enabling all stats using the global api for setting stats on
#   named component
#    - rate (events), resetOnOutput, local load-level using global api

# StatType0, StatType1 and StatType2 components test the following:
# - New explicit sst.Statistic use
#    - Sharing of same statistic by two stat slots

# StatGroup0, StatGroup1 and StatGroup2 test the following:
# - Statistic Groups
#    - Output for stat groups
#    - Enabling stats through stats groups

########################################################################
########################################################################

# Set the partitioner to be round robin so elements of the same type
# will get scattered across partitions
sst.setProgramOptions({
    "partitioner" : "roundrobin"
})

# Set the Statistic Load Level; Statistics with Enable Levels (set in
# elementInfoStatistic) lower or equal to the load can be enabled (default = 0)
sst.setStatisticLoadLevel(2)

# Set the desired Statistic Output (sst.statOutputConsole is default)
sst.setStatisticOutput("sst.statOutputConsole", {
    "outputsimtime" : True,
    "outputrank" : False
})

#sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : sys.argv[1],
#                                             "outputrank" : False
#                                            })

#sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "./TestOutput.csv",
#			                                 "separator" : ", "
#                                            })

########################################################################
########################################################################

# Define the simulation components

# NOTE: using a count that is a prime number because there are some
# oddities in parallel versus serial runs for the case when a periodic
# output happens on the same cycle as simulation ends.  For serial,
# both the periodic and end outputs happen.  In parallel, only the end
# output prints, which makes the diff fail between serial and parallel
# runs.  This only occurs here because we have no links so all the
# ranks run independently.  This does not seem to happen for runs that
# have a proper synchronization happening (i.e. there are links that
# cross partition boundaries).

# These first two components are to test
# enableAllStatisticsForAllComponents
StatGlobal0 = sst.Component("StatGlobal0", "coreTestElement.StatisticsComponent.int")
StatGlobal0.addParams({
      "rng" : "marsaglia",
      "count" : "101",   # Change For number of 1ns clocks
      "seed_w" : "1440",
      "seed_z" : "1046"
})

StatGlobal1 = sst.Component("StatGlobal1", "coreTestElement.StatisticsComponent.int")
StatGlobal1.addParams({
      "rng" : "marsaglia",
      "count" : "101",   # Change For number of 1ns clocks
      "seed_w" : "1441",
      "seed_z" : "1047"
})
StatGlobal1.setStatisticLoadLevel(4)

# This will only enable stats for Components that are already created
sst.enableAllStatisticsForAllComponents({
    "type" : "sst.AccumulatorStatistic",
    "rate" : "8 ns",
    "startat" : "50 ns"
})

# The next components test the basic functionality of enabling
# statistics on specific components

# Object 0
StatBasic0 = sst.Component("StatBasic0", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic0.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1447",
      "seed_z" : "1053"
})

# Enable Individual Statistics for the Component with separate rates
StatBasic0.enableStatistics(["stat1_U32"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "5 ns"})

StatBasic0.enableStatistics(["stat2_U64"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "10 ns",
    "resetOnOutput" : True})

StatBasic0.enableStatistics(["stat3_I32"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "7 ns",
    "startat" : "25 ns",
    "stopat" : "60 ns"})

StatBasic0.enableStatistics(["stat4_I64"], {
    "type" : "sst.AccumulatorStatistic",
    "startat" : "35 ns",
    "stopat" : "70 ns",
    "rate" : "12 ns",
    "resetOnOutput" : True})


# Object 1
StatBasic1 = sst.Component("StatBasic1", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic1.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1448",
      "seed_z" : "1054"
})

# Enable Individual Statistics for the Component with separate rates
StatBasic1.enableStatistics(["stat1_U32"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "6 event"})

StatBasic1.enableStatistics(["stat2_U64"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "11 events",
    "resetOnOutput" : True})

StatBasic1.enableStatistics(["stat3_I32"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "8 events",
    "startat" : "25 ns",
    "stopat" : "60 ns"})

StatBasic1.enableStatistics(["stat4_I64"], {
    "type" : "sst.AccumulatorStatistic",
    "startat" : "35 ns",
    "stopat" : "70 ns",
    "rate" : "14 events",
    "resetOnOutput" : True})


# Object 2
StatBasic2 = sst.Component("StatBasic2", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic2.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1449",
      "seed_z" : "1055"
})

# Enable Individual Statistics for the Component with separate rates
StatBasic2.enableStatistics(["stat1_U32", "stat3_I32"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "20 ns"})

StatBasic2.enableStatistics(["stat2_U64", "stat4_I64"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "16 ns",
    "startat" : "25 ns",
    "stopat" : "60 ns",
    "resetOnOutput" : True})


# Object 3
StatBasic3 = sst.Component("StatBasic3", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic3.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1450",
      "seed_z" : "1056"
})

# Enable Individual Statistics for the Component with separate rates
StatBasic3.enableStatistics(["stat1_U32", "stat3_I32"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "9 events"})

StatBasic3.enableStatistics(["stat2_U64", "stat4_I64"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "13 events",
    "startat" : "25 ns",
    "stopat" : "60 ns",
    "resetOnOutput" : True})


# Object 4
StatBasic4 = sst.Component("StatBasic4", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic4.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1451",
      "seed_z" : "1057"
})

# Enable Individual Statistics for the Component with separate rates
sst.enableStatisticForComponentName("StatBasic4", "stat1_U32", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "5 ns"})

sst.enableStatisticForComponentName("StatBasic4", "stat2_U64", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "10 ns",
    "resetOnOutput" : True})

sst.enableStatisticsForComponentName("StatBasic4", "stat3_I32", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "7 ns",
    "startat" : "25 ns",
    "stopat" : "60 ns"})

sst.enableStatisticsForComponentName("StatBasic4", ["stat4_I64"], {
    "type" : "sst.AccumulatorStatistic",
    "startat" : "35 ns",
    "stopat" : "70 ns",
    "rate" : "12 ns",
    "resetOnOutput" : True})


# Object 5
StatBasic5 = sst.Component("StatBasic5", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic5.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1452",
      "seed_z" : "1058"
})

# Enable Individual Statistics for the Component with separate rates
sst.enableStatisticForComponentName("StatBasic5", "stat1_U32", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "6 event"})

sst.enableStatisticForComponentName("StatBasic5", "stat2_U64", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "11 events",
    "resetOnOutput" : True})

sst.enableStatisticsForComponentName("StatBasic5", "stat3_I32", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "8 events",
    "startat" : "25 ns",
    "stopat" : "60 ns"})

sst.enableStatisticsForComponentName("StatBasic5", ["stat4_I64"], {
    "type" : "sst.AccumulatorStatistic",
    "startat" : "35 ns",
    "stopat" : "70 ns",
    "rate" : "14 events",
    "resetOnOutput" : True})


# Object 6
StatBasic6 = sst.Component("StatBasic6", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic6.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1453",
      "seed_z" : "1059"
})

# Enable Individual Statistics for the Component with separate rates
sst.enableStatisticsForComponentName("StatBasic6", ["stat1_U32", "stat3_I32"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "20 ns"})

sst.enableStatisticsForComponentName("StatBasic6", ["stat2_U64", "stat4_I64"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "16 ns",
    "startat" : "25 ns",
    "stopat" : "60 ns",
    "resetOnOutput" : True})


# Object 7
StatBasic7 = sst.Component("StatBasic7", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic7.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1454",
      "seed_z" : "1060"
})

# Enable Individual Statistics for the Component with separate rates
sst.enableStatisticsForComponentName("StatBasic7", ["stat1_U32", "stat3_I32"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "9 events"})

sst.enableStatisticsForComponentName("StatBasic7", ["stat2_U64", "stat4_I64"], {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "13 events",
    "startat" : "25 ns",
    "stopat" : "60 ns",
    "resetOnOutput" : True})


# The next components test enabling all statistics in a specific
# component along with setting the statistic load level

# Object 8
StatBasic8 = sst.Component("StatBasic8", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic8.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1455",
      "seed_z" : "1061"
})

StatBasic8.enableAllStatistics({
    "type" : "sst.AccumulatorStatistic",
    "rate" : "20 ns" })


# Object 9
StatBasic9 = sst.Component("StatBasic9", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic9.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1456",
      "seed_z" : "1062"
})

StatBasic9.enableAllStatistics({
    "type" : "sst.AccumulatorStatistic",
    "rate" : "25 ns" })

StatBasic9.setStatisticLoadLevel(4)


# Object 10
StatBasic10 = sst.Component("StatBasic10", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatBasic10.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1457",
      "seed_z" : "1063"
})

sst.enableAllStatisticsForComponentName("StatBasic10", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "25 ns" })

sst.setStatisticLoadLevelForComponentName("StatBasic10", 1)


# The following components test sst.enableAllStatisticsForComponentType()
StatType0 = sst.Component("StatType0", "coreTestElement.StatisticsComponent.float")

# Set Component Parameters
StatType0.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1457",
      "seed_z" : "1063"
})

StatType1 = sst.Component("StatType1", "coreTestElement.StatisticsComponent.float")

# Set Component Parameters
StatType1.addParams({
    "rng" : "marsaglia",
    "count" : "101",
    "seed_w" : "1457",
    "seed_z" : "1063"
})

StatType2 = sst.Component("StatType2", "coreTestElement.StatisticsComponent.float")

# Set Component Parameters
StatType2.addParams({
    "rng" : "marsaglia",
    "count" : "101",
    "seed_w" : "1457",
    "seed_z" : "1063"
})

sst.enableAllStatisticsForComponentType("coreTestElement.StatisticsComponent.float", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "20 ns" })

# Set load level for all components of type
sst.setStatisticLoadLevelForComponentType("coreTestElement.StatisticsComponent.float", 1)

# Set a different load level for StatType2
StatType2.setStatisticLoadLevel(2)


# The next components test the new interface using explicit stats
# objects (sst.Statistic)

StatObjComp0 = sst.Component("StatObjComp0", "coreTestElement.StatisticsComponent.float")

# Set Component Parameters
StatObjComp0.addParams({
    "rng" : "marsaglia",
    "count" : "101",
    "seed_w" : "1458",
    "seed_z" : "1064"
})

stat = StatObjComp0.createStatistic("statobj0", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "17ns"
})

StatObjComp0.setStatistic("stat2_F64", stat)


# This component tests sharing of stats by multiple stat slots
StatObjComp1 = sst.Component("StatObjComp1", "coreTestElement.StatisticsComponent.float")

# Set Component Parameters
StatObjComp1.addParams({
    "rng" : "marsaglia",
    "count" : "101",
    "seed_w" : "1459",
    "seed_z" : "1065"
})

stat = StatObjComp1.createStatistic("statobj1", {
    "type" : "sst.AccumulatorStatistic",
    "rate" : "17ns"
})

StatObjComp1.setStatistic("stat2_F64", stat)
StatObjComp1.setStatistic("stat3_F64", stat)


# Statistic Group testing.  There are three tests:

# Test 1 - Console output with 1 group of 3 components
# Test 2 - CSV output with 1 group of 3 components
# Test 3 - TXT output with 2 groups of 3 components

##### StatGroup0 - Console
StatGroup0 = sst.StatisticGroup("StatGroup0")

StatGroupObj0 = sst.Component("StatGroupObj0", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj0.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1460",
      "seed_z" : "1066"
})

# This component will test stat groups
StatGroupObj1 = sst.Component("StatGroupObj1", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj1.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1461",
      "seed_z" : "1067"
})

# This component will test stat groups
StatGroupObj2 = sst.Component("StatGroupObj2", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj2.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1462",
      "seed_z" : "1068"
})


# Add the components
StatGroup0.addComponent(StatGroupObj0);
StatGroup0.addComponent(StatGroupObj1);
StatGroup0.addComponent(StatGroupObj2);

# Add the stats
StatGroup0.addStatistic("stat1_U32", {
    "type" : "sst.AccumulatorStatistic"})
StatGroup0.addStatistic("stat2_U64", {
    "type" : "sst.AccumulatorStatistic",
    "resetOnOutput" : True})

# Set dump frequency
StatGroup0.setFrequency("19ns")

# Set output
StatOutput0 = sst.StatisticOutput("sst.statOutputConsole", {
    "outputsimtime" : True,
    "outputrank" : False
})

StatGroup0.setOutput(StatOutput0)


##### StatGroup1 - CSV
StatGroup1 = sst.StatisticGroup("StatGroup1")

StatGroupObj3 = sst.Component("StatGroupObj3", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj3.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1463",
      "seed_z" : "1069"
})

# This component will test stat groups
StatGroupObj4 = sst.Component("StatGroupObj4", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj4.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1464",
      "seed_z" : "1070"
})

# This component will test stat groups
StatGroupObj5 = sst.Component("StatGroupObj5", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj5.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1466",
      "seed_z" : "1071"
})


# Add the components
StatGroup1.addComponent(StatGroupObj3);
StatGroup1.addComponent(StatGroupObj4);
StatGroup1.addComponent(StatGroupObj5);

# Add the stats
StatGroup1.addStatistic("stat1_U32", {
    "type" : "sst.AccumulatorStatistic"})
StatGroup1.addStatistic("stat2_U64", {
    "type" : "sst.AccumulatorStatistic",
    "resetOnOutput" : True})

# Set dump frequency
StatGroup1.setFrequency("23ns")

# Set output
StatOutput1 = sst.StatisticOutput("sst.statOutputCSV", {
    "filepath" : "test_StatisticsComponent_basic_group_stats.csv",
    "separator" : ", ",
    "outputrank" : "0"
})

StatGroup1.setOutput(StatOutput1)


##### StatGroup2 & StatGroup3 - TXT

## StatGroup2
StatGroup2 = sst.StatisticGroup("StatGroup2")

StatGroupObj6 = sst.Component("StatGroupObj6", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj6.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1466",
      "seed_z" : "1072"
})

# This component will test stat groups
StatGroupObj7 = sst.Component("StatGroupObj7", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj7.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1467",
      "seed_z" : "1073"
})

# This component will test stat groups
StatGroupObj8 = sst.Component("StatGroupObj8", "coreTestElement.StatisticsComponent.int")

# Set Component Parameters
StatGroupObj8.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1468",
      "seed_z" : "1074"
})


# Add the components
StatGroup2.addComponent(StatGroupObj6);
StatGroup2.addComponent(StatGroupObj7);
StatGroup2.addComponent(StatGroupObj8);

# Add the stats
StatGroup2.addStatistic("stat1_U32", {
    "type" : "sst.AccumulatorStatistic"})
StatGroup2.addStatistic("stat2_U64", {
    "type" : "sst.AccumulatorStatistic",
    "resetOnOutput" : True})

# Set dump frequency
StatGroup2.setFrequency("27ns")

# Set output
StatOutput2 = sst.StatisticOutput("sst.statOutputTXT", {
    "filepath" : "test_StatisticsComponent_basic_group_stats.txt",
    "outputrank" : "0"
})

StatGroup2.setOutput(StatOutput2)

## StatGroup3 (same output as StatGroup2)
StatGroup3 = sst.StatisticGroup("StatGroup3")

StatGroupObj9 = sst.Component("StatGroupObj9", "coreTestElement.StatisticsComponent.float")

# Set Component Parameters
StatGroupObj9.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1469",
      "seed_z" : "1075"
})

# This component will test stat groups
StatGroupObj10 = sst.Component("StatGroupObj10", "coreTestElement.StatisticsComponent.float")

# Set Component Parameters
StatGroupObj10.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1470",
      "seed_z" : "1076"
})

# This component will test stat groups
StatGroupObj11 = sst.Component("StatGroupObj11", "coreTestElement.StatisticsComponent.float")

# Set Component Parameters
StatGroupObj11.addParams({
      "rng" : "marsaglia",
      "count" : "101",
      "seed_w" : "1471",
      "seed_z" : "1077"
})


# Add the components
StatGroup3.addComponent(StatGroupObj9);
StatGroup3.addComponent(StatGroupObj10);
StatGroup3.addComponent(StatGroupObj11);

# Add the stats
StatGroup3.addStatistic("stat1_F32", {
    "type" : "sst.AccumulatorStatistic"})
StatGroup3.addStatistic("stat2_F64", {
    "type" : "sst.AccumulatorStatistic",
    "resetOnOutput" : True})

# Set dump frequency
StatGroup3.setFrequency("29ns")

# Set output
StatGroup3.setOutput(StatOutput2)
