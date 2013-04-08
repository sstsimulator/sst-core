// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization/core.h"

#include <sst/core/sst_types.h>
#include <sst/core/debug.h>
#include <sst/core/timeLord.h>
#include <cstdio>
#include <cstring>

namespace SST {


TimeConverter* TimeLord::getTimeConverter(std::string ts) {
    // See if this is in the cache
    if ( parseCache.find(ts) == parseCache.end() ) {
	SimTime_t simCycles = getSimCycles(ts, "getTimeConverter()");
	TimeConverter* tc = getTimeConverter(simCycles);
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
    

TimeLord::TimeLord(std::string timeBaseString) :
    timeBaseString(timeBaseString)
{

    // Convert timeBaseString into the factor that will be used to
    // parse strings to convert TimeConverters.  This is quick and
    // dirty (and relatively ugly); should probably be improved later.
    double value;
    char si[2];
    char unit[3];
    char tmp[2];

    const char *tbs_char = timeBaseString.c_str();
    int ret = sscanf(tbs_char,"%lf %1s %2s %1s",&value,si,unit,tmp);
    if ( ret <= 1 || ret > 3) {
	_abort(TimeLord,"Base time format error: %s\n",tbs_char);
    }

    // Do seconds with no SI unit as special case
    if ( ret == 2 ) {
	// Only valid string at this point is one that specified seconds
	if ( si[0] == 's' ) {
	    // Numbers will be converted to attoseconds, so we need to
	    // find the divisor that will convert to timesteps in the
	    // simulator core running at the chosen time base.  That
	    // really just means we convert this value to attoseconds.
	    sec_factor = value * 1000000000000000000.0;
	}
	else {
	    _abort(TimeLord,"Base time format error: %s\n",tbs_char);
	}
    }
    
    if ( !strcmp("s",unit ) ) {
	// Numbers will be converted to attoseconds, so we need to
	// find the divisor that will convert to timesteps in the
	// simulator core running at the chosen time base.  That
	// really just means we convert this value to attoseconds.
	switch (si[0]) {
	case 'f':
	    sec_factor = value * 1000.0;
	    break;
	case 'p':
	    sec_factor = value * 1000000.0;
	    break;
	case 'n':
	    sec_factor = value * 1000000000.0;
	    break;
	case 'u':
	    sec_factor = value * 1000000000000.0;
	    break;
	case 'm':
	    sec_factor = value * 1000000000000000.0;
	    break;
        default:
	    _abort(TimeLord,"Base time format error (unregonized SI unit): %s\n",tbs_char);
      }
    }
    else {
	_abort(TimeLord,"Base time format error: %s\n",tbs_char);
    }

    nano = getTimeConverter("1ns");
    micro = getTimeConverter("1us");
    milli = getTimeConverter("1ms");
}

TimeLord::~TimeLord() {
    // Delete all the TimeConverter objects
    std::map<ComponentId_t,LinkMap*>::iterator it;
    for ( TimeConverterMap_t::iterator it = tcMap.begin(); it != tcMap.end(); ++it ) {
	delete it->second;
    }
    tcMap.clear();

    // Clear the contents of the cache
    parseCache.clear();
    
    
}

SimTime_t TimeLord::getSimCycles(std::string timeString, std::string where) {

    // Parse the string and based on the value create and/or return
    // the appropriate TimeConverter object.
    SimTime_t simCycles = 0;

    double value;
    char si[2];
    char unit[3];
    char tmp[2];

    // Convert string to char* so we can use scanf
    const char *ts = timeString.c_str();
    
    int ret = sscanf(ts,"%lf %1s %2s %1s",&value,si,unit,tmp);
    if ( ret <= 1 || ret > 3) {
	_abort(TimeLord,
	       "getTimeConverter(): Format error: \"%s\" in %s\n\tExpected \"%%lf %%1s %%2s %%1s\"\n",ts, 
	       where.c_str());
    }

    // Do seconds and hertz with no SI unit as special case
    if ( ret == 2 ) {
	// Only valid string at this point is one that specified seconds
	if ( si[0] == 's' ) {
	    simCycles = (SimTime_t)(value * 1000000000000000000.0 / sec_factor);
	}
	else {
	    _abort(TimeLord,"getTimeConverter(): Format error: \"%s\" in %s\n",
		   ts,
		   where.c_str());
	}
    }
    else {
	if ( (si[0] == 'H' || si[0] == 'h') && !strcmp("z",unit) ) {
	    simCycles = (SimTime_t)(1000000000000000000.0 / sec_factor / value);
	}
    }
    
    if ( !strcmp("s",unit ) ) {
      // Convert to number of picoseconds
      switch (si[0]) {
	case 'f':
	    // First convert to number of attoseconds
	    simCycles = (SimTime_t)(value * 1000.0 / sec_factor);
	  break;
	case 'p':
	    // First convert to number of attoseconds
	    simCycles = (SimTime_t)(value * 1000000.0 / sec_factor);
	  break;
	case 'n':
	    simCycles = (SimTime_t)(value * 1000000000.0 / sec_factor);
	    break;
	case 'u':
	    simCycles = (SimTime_t)(value * 1000000000000.0 / sec_factor);
	    break;
	case 'm':
	    simCycles = (SimTime_t)(value * 1000000000000000.0 / sec_factor);
	    break;
        default:
	    _abort(TimeLord,"getTimeConverter(): Format error (unrecognized SI unit): \"%s\"\n",ts);
      }
    }
    else if ( !strcmp("Hz",unit) || !strcmp("hz",unit) ) {
	// Convert to a period in number of picoseconds
	switch (si[0] ) {
	case 'k':
	    simCycles = (SimTime_t)(1000000000000000.0 / sec_factor / value);
	  break;
	case 'M':
	    simCycles = (SimTime_t)(1000000000000.0 / sec_factor / value);
	  break;
	case 'G':
	    simCycles = (SimTime_t)(1000000000.0 / sec_factor / value);
	    break;
        default:
	    _abort(TimeLord,"getTimeConverter(): Format error (unrecognized SI unit): \"%s\"\n",ts);
      }
    }
    else if ( ret != 2 ){
	_abort(TimeLord,"getTimeConverter(): Format error: \"%s\" in %s\n",
	       ts, where.c_str());
    }

    return simCycles;
     
    
}



} // namespace SST
