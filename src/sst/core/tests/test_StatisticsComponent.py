import sst

########################################################################
# This test file shows examples of multiple ways of enabling Statistics 
# and their associated parameters.  Additionally, it shows different ways
# to set the Statistic Output and its parameters.  All of the methods 
# are available, but most are commented out.  The user can adjust the file
# as need be to see the operation of the various commands
########################################################################

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "200ns")

########################################################################
########################################################################

# Set the Statistic Load Level; Statistics with Enable Levels (set in 
# elementInfoStatistic) lower or equal to the load can be enabled (default = 0)
sst.setStatisticLoadLevel(7)   

# Set the desired Statistic Output (sst.statOutputConsole is default)
#sst.setStatisticOutput("sst.statOutputConsole")         
#sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : "./TestOutput.txt"
#                                            })              
sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "./TestOutput.csv",    
			                                 "separator" : ", "
                                            })              

#sst.setStatisticOutputOptions({"outputtopheader" : "1",
#                               "outputinlineheader" : "1",
#                               "outputsimtime": "1",
#                               "outputrank": "1",
#                               "help" : "help" })
#sst.setStatisticOutputOption("outputtopheader", "1")
#sst.setStatisticOutputOption("outputinlineheader", "1")
#sst.setStatisticOutputOption("outputsimtime", "1")
#sst.setStatisticOutputOption("outputrank", "1")
#sst.setStatisticOutputOption("help", "help")

########################################################################
########################################################################

######################################
# RANK 0 Component + Statistic Enables

# Define the simulation components
StatExample0 = sst.Component("simpleStatisticsTest0", "simpleElementExample.simpleStatisticsComponent")
StatExample0.setRank(0)

# Set Component Parameters
StatExample0.addParams({
      "rng" : """marsaglia""",
      "count" : """100""",   # Change For number of 1ns clocks
      "seed_w" : """1447""",
      "seed_z" : """1053"""
})

## # Enable ALL Statistics for the Component to output at end of sim
## # Statistic defaults to Accumulator
## StatExample0.enableAllStatistics()
                                  
## # Enable ALL Statistics for the Component
## StatExample0.enableAllStatistics({ "type":"sst.AccumulatorStatistic",
##                                    "rate":"10 ns"
##                                  })

## # Enable Individual Statistics for the Component with output at end of sim
## # Statistic defaults to Accumulator
## StatExample0.enableStatistics([
##       "stat1_U32",
##       "stat2_U64",
##       "stat3_I32",
##       "stat4_I64", 
##       "stat5_U32",
##       "stat6_U64"
## ])

## # Enable Individual Statistics for the Component with the same rate
## StatExample0.enableStatistics([
##       "stat1_U32",
##       "stat2_U64",
##       "stat3_I32",
##       "stat4_I64", 
##       "stat5_U32",
##       "stat6_U64"
## ], 
## { "type":"sst.AccumulatorStatistic",
##   "rate":"100 ns"}
## )

# Enable Individual Statistics for the Component with separate rates
StatExample0.enableStatistics([
      "stat1_U32"], {
      "type":"sst.HistogramStatistic",
      "minvalue" : "10", 
      "binwidth" : "10", 
      "numbins"  : "41", 
      "IncludeOutOfBounds" : "1", 
      "rate":"5 ns"})

StatExample0.enableStatistics([
      "stat2_U64"], {
      "type":"sst.HistogramStatistic",
      "minvalue" : "1000", 
      "binwidth" : "1000", 
      "numbins"  : "17", 
      "IncludeOutOfBounds" : "1", 
      "rate":"10 ns"})
            
StatExample0.enableStatistics([
      "stat3_I32"], {
      "type":"sst.HistogramStatistic",
      "minvalue" : "-200", 
      "binwidth" : "50", 
      "numbins"  : "8", 
      "IncludeOutOfBounds" : "1", 
      "rate":"25 events"})

StatExample0.enableStatistics([
      "stat4_I64"], {
      "type":"sst.HistogramStatistic",
      "minvalue" : "-9000", 
      "binwidth" : "1000", 
      "numbins"  : "18", 
      "IncludeOutOfBounds" : "1", 
      "rate":"50 ns"})

StatExample0.enableStatistics([
      "stat5_U32"], {
      "type":"sst.AccumulatorStatistic",
      "rate":"5 ns"
      })

StatExample0.enableStatistics([
      "stat6_U64"], {
      "type":"sst.AccumulatorStatistic",
      "rate":"10 ns",
      "startat":"35ns",
      "stopat":"65ns"
      })

########################################################################
########################################################################

######################################
#### # RANK 1 Component + Statistic Enables
#### 
#### # Define the simulation components
#### StatExample1 = sst.Component("simpleStatisticsTest1", "simpleElementExample.simpleStatisticsComponent")
#### StatExample1.setRank(1)
#### 
#### # Set Component Parameters
#### StatExample1.addParams({
####       "rng" : """marsaglia""",
####       "count" : """100""",   # Change For number of 1ns clocks
####       "seed_w" : """1447""",
####       "seed_z" : """1053"""
#### })
#### 
#### 
#### 
#### ## # Enable ALL Statistics for the Component to output at end of sim
#### ## # Statistic defaults to Accumulator
#### StatExample1.enableAllStatistics()
####                                    
#### ## # Enable ALL Statistics for the Component
#### ## StatExample1.enableAllStatistics({ "type":"sst.AccumulatorStatistic",
#### ##                                    "rate":"100 ns"
#### ##                                  })
#### 
#### ## # Enable Individual Statistics for the Component with output at end of sim
#### ## # Statistic defaults to Accumulator
#### ## StatExample1.enableStatistics([
#### ##       "stat1_U32",
#### ##       "stat2_U64",
#### ##       "stat3_I32",
#### ##       "stat4_I64", 
#### ##       "stat5_U32",
#### ##       "stat6_U64"
#### ## ])
#### 
#### ## # Enable Individual Statistics for the Component with the same rate
#### ## StatExample1.enableStatistics([
#### ##       "stat1_U32",
#### ##       "stat2_U64",
#### ##       "stat3_I32",
#### ##       "stat4_I64", 
#### ##       "stat5_U32",
#### ##       "stat6_U64"
#### ## ], 
#### ## { "type":"sst.AccumulatorStatistic",
#### ##   "rate":"100 ns"}
#### ## )
#### 
#### # Enable Individual Statistics for the Component with separate rates
#### StatExample1.enableStatistics([
####       "stat1_U32"], {
####       "type":"sst.HistogramStatistic",
####       "minvalue" : "10", 
####       "binwidth" : "10", 
####       "numbins"  : "41", 
####       "IncludeOutOfBounds" : "1", 
####       "rate":"5 ns"})
#### 
#### StatExample1.enableStatistics([
####       "stat2_U64"], {
####       "type":"sst.HistogramStatistic",
####       "minvalue" : "1000", 
####       "binwidth" : "1000", 
####       "numbins"  : "17", 
####       "IncludeOutOfBounds" : "1", 
####       "rate":"10 ns"})
####             
#### StatExample1.enableStatistics([
####       "stat3_I32"], {
####       "type":"sst.HistogramStatistic",
####       "minvalue" : "-200", 
####       "binwidth" : "50", 
####       "numbins"  : "8", 
####       "IncludeOutOfBounds" : "1", 
####       "rate":"25 events"})
#### 
#### StatExample1.enableStatistics([
####       "stat4_I64"], {
####       "type":"sst.HistogramStatistic",
####       "minvalue" : "-9000", 
####       "binwidth" : "1000", 
####       "numbins"  : "18", 
####       "IncludeOutOfBounds" : "1", 
####       "rate":"50 ns"})
#### 
#### StatExample1.enableStatistics([
####       "stat5_U32"], {
####       "type":"sst.AccumulatorStatistic",
####       "rate":"5 ns"})
#### 
#### StatExample1.enableStatistics([
####       "stat6_U64"], {
####       "type":"sst.AccumulatorStatistic",
####       "rate":"10 ns"})
#### 

########################################################################
########################################################################
########################################################################
########################################################################

## Globally Enable Statistics (Must Occur After Components Are Created)

#### # Enable ALL Statistics for ALL Components with output at end of sim
#### # Statistic defaults to Accumulator
#### sst.enableAllStatisticsForAllComponents()

#### # Enable ALL Statistics for ALL Components
#### sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic",
####                                          "rate":"10 ns"})

#### # Enable ALL Statistics for Components defined by Name with output at end of sim
#### # Statistic defaults to Accumulator
#### sst.enableAllStatisticsForComponentName("simpleStatisticsTest0")
#### sst.enableAllStatisticsForComponentName("simpleStatisticsTest1")

#### # Enable ALL Statistics for Components defined by Name
#### sst.enableAllStatisticsForComponentName("simpleStatisticsTest0", 
####                                        {"type":"sst.AccumulatorStatistic",
####                                         "rate":"10 ns"})
#### 
#### sst.enableAllStatisticsForComponentName("simpleStatisticsTest1", 
####                                        {"type":"sst.AccumulatorStatistic",
####                                         "rate":"20 ns"})


#### # Enable ALL Statistics for Components defined by Type with output at end of sim
#### sst.enableAllStatisticsForComponentType("simpleStatistics.simpleStatistics")

#### # Enable ALL Statistics for Components defined by Type
#### sst.enableAllStatisticsForComponentType("simpleStatistics.simpleStatistics", 
####                                        {"type":"sst.AccumulatorStatistic",
####                                         "rate":"10 ns"})


#### # Enable Single Statistic for Components defined by Name with output at end of sim
#### sst.enableStatisticForComponentName("simpleStatisticsTest0", "stat1_U32")
#### sst.enableStatisticForComponentName("simpleStatisticsTest1", "stat2_U64")

#### # Enable Single Statistic for Components defined by Name
#### sst.enableStatisticForComponentName("simpleStatisticsTest0", "stat1_U32", 
####                                    {"type":"sst.HistogramStatistic",
####                                     "minvalue" : "10",
####                                     "binwidth" : "10", 
####                                     "numbins"  : "41", 
####                                     "IncludeOutOfBounds" : "1", 
####                                     "rate":"10 ns"})
#### sst.enableStatisticForComponentName("simpleStatisticsTest1", "stat3_I32", 
####                                    {"type":"sst.HistogramStatistic",
####                                     "minvalue" : "-200", 
####                                     "binwidth" : "50", 
####                                     "numbins"  : "8", 
####                                     "IncludeOutOfBounds" : "1", 
####                                     "rate":"100 events"})
#### sst.enableStatisticForComponentName("simpleStatisticsTest1", "stat4_I64", 
####                                    {"type":"sst.HistogramStatistic",
####                                     "minvalue" : "-9000", 
####                                     "binwidth" : "1000", 
####                                     "numbins"  : "18", 
####                                     "IncludeOutOfBounds" : "1", 
####                                     "rate":"20 ns"})
#### sst.enableStatisticForComponentName("simpleStatisticsTest1", "stat6_U64", 
####                                    {"type":"sst.AccumulatorStatistic",
####                                     "rate":"30 ns"})

#### # Enable Single Statistic for Components defined by Type with output at end of sim
#### sst.enableStatisticForComponentType("simpleStatistics.simpleStatistics", "stat1_U32")
#### sst.enableStatisticForComponentType("simpleStatistics.simpleStatistics", "stat2_U64")

#### # Enable Single Statistic for Components defined by Type 
#### sst.enableStatisticForComponentType("simpleStatistics.simpleStatistics", "stat1_U32", 
####                                    {"type":"sst.HistogramStatistic",
####                                     "minvalue" : "10",
####                                     "binwidth" : "10", 
####                                     "numbins"  : "41", 
####                                     "IncludeOutOfBounds" : "1", 
####                                     "rate":"5 ns"})
#### sst.enableStatisticForComponentType("simpleStatistics.simpleStatistics", "stat3_I32", 
####                                    {"type":"sst.HistogramStatistic",
####                                     "minvalue" : "-200", 
####                                     "binwidth" : "50", 
####                                     "numbins"  : "8", 
####                                     "IncludeOutOfBounds" : "1", 
####                                     "rate":"100 events"})
#### sst.enableStatisticForComponentType("simpleStatistics.simpleStatistics", "stat4_I64", 
####                                    {"type":"sst.HistogramStatistic",
####                                     "minvalue" : "-9000", 
####                                     "binwidth" : "1000", 
####                                     "numbins"  : "18", 
####                                     "IncludeOutOfBounds" : "1", 
####                                     "rate":"20 ns"})
#### sst.enableStatisticForComponentType("simpleStatistics.simpleStatistics", "stat6_U64", 
####                                    {"type":"sst.AccumulatorStatistic",
####                                     "rate":"30 ns"})

