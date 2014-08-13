// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization.h"
#include <sst/core/timeLord.h>

#include <cstdio>
#include <cstring>

#include <sst/core/output.h>
#include <sst/core/linkMap.h>
#include <sst/core/timeConverter.h>

namespace SST {


TimeConverter* TimeLord::getTimeConverter(std::string ts) {
    // See if this is in the cache
    if ( parseCache.find(ts) == parseCache.end() ) {
        TimeConverter* tc = getTimeConverter(UnitAlgebra(ts));
        parseCache[ts] = tc;
        return tc;
    }
    return parseCache[ts];
}

TimeConverter* TimeLord::getTimeConverter(SimTime_t simCycles) {
    // Check to see if we already have a TimeConverter with this value
    if ( tcMap.find(simCycles) == tcMap.end() ) {
        TimeConverter *tc = new TimeConverter(simCycles);
        tcMap[simCycles] = tc;
        return tc;
    }
    return tcMap[simCycles];
}

TimeConverter* TimeLord::getTimeConverter(const UnitAlgebra& ts) {
    SimTime_t simCycles;
    UnitAlgebra period = ts;
    UnitAlgebra uaFactor;
    // Need to differentiate between Hz and s.
    if ( period.hasUnits("s") ) {
        // simCycles = (period / timeBase).getRoundedValue();
        uaFactor = period / timeBase;
    }
    else if ( period.hasUnits("Hz") ) {
        UnitAlgebra temp = timeBase;
        // simCycles = (temp.invert() / period).getRoundedValue();
        uaFactor = temp.invert() / period;
    }
    else {
        Output abort = Simulation::getSimulation()->getSimulationOutput();
        abort.fatal(CALL_INFO,1,"Error:  TimeConverter creation requires "
                    "a time unit (s or Hz), %s was passed to call\n",
                    ts.toString().c_str());
    }
    // Check to see if number is too big or too small
    if ( uaFactor.getValue() > MAX_SIMTIME_T ) {
        Output abort = Simulation::getSimulation()->getSimulationOutput();
        abort.fatal(CALL_INFO,1,"Error:  Attempting to get TimeConverter for a time (%s) "
                    "which is too large for the timebase (%s)\n",
                    ts.toString().c_str(),timeBase.toStringBestSI().c_str());

    }
    simCycles = uaFactor.getRoundedValue();
    TimeConverter* tc = getTimeConverter(simCycles);
    return tc;
}

TimeLord::TimeLord(std::string timeBaseString) :
    timeBaseString(timeBaseString)
{
    timeBase = UnitAlgebra(timeBaseString);

    nano = getTimeConverter("1ns");
    micro = getTimeConverter("1us");
    milli = getTimeConverter("1ms");
}

TimeLord::~TimeLord() {
    // Delete all the TimeConverter objects
    std::map<ComponentId_t,LinkMap*>::iterator it;
std::cerr << "       TimeLord desconstructor start loop" << std::endl;
    for ( TimeConverterMap_t::iterator it = tcMap.begin(); it != tcMap.end(); ++it ) {
	delete it->second;
    }
std::cerr << "  Finished loop" << std::endl;
    tcMap.clear();

std::cerr << "  next is Cache " << std::endl;
    // Clear the contents of the cache
    parseCache.clear();
    
    
}

SimTime_t TimeLord::getSimCycles(std::string ts, std::string where)
{
    // See if this is in the cache
    if ( parseCache.find(ts) == parseCache.end() ) {
        TimeConverter* tc = getTimeConverter(UnitAlgebra(ts));
        parseCache[ts] = tc;
        return tc->getFactor();
    }
    return parseCache[ts]->getFactor();
}

} // namespace SST
