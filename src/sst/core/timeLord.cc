// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/timeLord.h"

#include "sst/core/linkMap.h"
#include "sst/core/output.h"
#include "sst/core/serialization/serialize.h"
#include "sst/core/serialization/serializer.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeConverter.h"
#include "sst/core/warnmacros.h"

#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace SST {

using Core::ThreadSafe::Spinlock;

TimeConverter*
TimeLord::getTimeConverter(const std::string& ts)
{
    // See if this is in the cache
    std::lock_guard<std::recursive_mutex> lock(slock);
    if ( parseCache.find(ts) == parseCache.end() ) {
        TimeConverter* tc = getTimeConverter(UnitAlgebra(ts));
        parseCache[ts]    = tc;
        return tc;
    }
    return parseCache[ts];
}

TimeConverter*
TimeLord::getTimeConverter(SimTime_t simCycles)
{
    // Check to see if we already have a TimeConverter with this value
    std::lock_guard<std::recursive_mutex> lock(slock);
    if ( tcMap.find(simCycles) == tcMap.end() ) {
        TimeConverter* tc = new TimeConverter(simCycles);
        tcMap[simCycles]  = tc;
        return tc;
    }
    return tcMap[simCycles];
}

TimeConverter*
TimeLord::getTimeConverter(const UnitAlgebra& ts)
{
    Output& abort = Output::getDefaultObject();
    if ( !initialized ) { abort.fatal(CALL_INFO, 1, "Time Lord has not yet been initialized!"); }
    SimTime_t   simCycles;
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
        uaFactor         = temp.invert() / period;
    }
    else {
        throw std::invalid_argument(
            "Error:  TimeConverter creation requires "
            "a time unit (s or Hz), " +
            ts.toStringBestSI() + " was passed to call");
        // abort.fatal(
        //     CALL_INFO, 1,
        //     "Error:  TimeConverter creation requires "
        //     "a time unit (s or Hz), %s was passed to call\n",
        //     ts.toString().c_str());
    }
    // Check to see if number is too big or too small
    if ( uaFactor.getValue() > static_cast<uint64_t>(MAX_SIMTIME_T) ) {
        throw std::overflow_error(
            "Error:  Attempting to get TimeConverter for a time (" + ts.toStringBestSI() +
            ") "
            "which is too large for the timebase (" +
            timeBase.toStringBestSI() + ")");
        // abort.fatal(
        //     CALL_INFO, 1,
        //     "Error:  Attempting to get TimeConverter for a time (%s) "
        //     "which is too large for the timebase (%s)\n",
        //     ts.toStringBestSI().c_str(), timeBase.toStringBestSI().c_str());
    }
    // Check to see if period is too short (0 is special cased to not fail)
    if ( uaFactor.getValue() < 1 && uaFactor.getValue() != 0 ) {
        throw std::underflow_error(
            "Error:  Attempting to get TimeConverter for a time (" + ts.toStringBestSI() +
            ") "
            "which has too small of a period to be represented by the timebase (" +
            timeBase.toStringBestSI() + ")");
        // abort.fatal(
        //     CALL_INFO, 1,
        //     "Error:  Attempting to get TimeConverter for a time (%s) "
        //     "which has too small of a period to be represented by the timebase (%s)\n",
        //     ts.toStringBestSI().c_str(), timeBase.toStringBestSI().c_str());
    }
    simCycles         = uaFactor.getRoundedValue();
    TimeConverter* tc = getTimeConverter(simCycles);
    return tc;
}

void
TimeLord::init(const std::string& _timeBaseString)
{
    initialized    = true;
    timeBaseString = _timeBaseString;
    timeBase       = UnitAlgebra(timeBaseString);

    try {
        nano = getTimeConverter("1ns");
    }
    catch ( std::underflow_error& e ) {
        // This means that the core timebase is too big to represent
        // this time. Just set it to nulllptr
        nano = nullptr;
    }

    try {
        micro = getTimeConverter("1us");
    }
    catch ( std::underflow_error& e ) {
        // This means that the core timebase is too big to represent
        // this time. Just set it to nulllptr
        micro = nullptr;
    }

    try {
        milli = getTimeConverter("1ms");
    }
    catch ( std::underflow_error& e ) {
        // This means that the core timebase is too big to represent
        // this time. Just set it to nulllptr
        milli = nullptr;
    }
}

TimeLord::~TimeLord()
{
    // Delete all the TimeConverter objects
    std::map<ComponentId_t, LinkMap*>::iterator it;
    for ( TimeConverterMap_t::iterator it = tcMap.begin(); it != tcMap.end(); ++it ) {
        delete it->second;
    }
    tcMap.clear();

    // Clear the contents of the cache
    parseCache.clear();
}

SimTime_t
TimeLord::getSimCycles(const std::string& ts, const std::string& UNUSED(where))
{
    // See if this is in the cache
    std::lock_guard<std::recursive_mutex> lock(slock);
    if ( parseCache.find(ts) == parseCache.end() ) {
        TimeConverter* tc = getTimeConverter(UnitAlgebra(ts));
        parseCache[ts]    = tc;
        return tc->getFactor();
    }
    return parseCache[ts]->getFactor();
}

void
TimeLord::serialize_order(SST::Core::Serialization::serializer& ser)
{
    ser& timeBaseString;
    ser& timeBase;
    ser& tcMap;
}

UnitAlgebra
TimeConverter::getPeriod() const
{
    return Simulation_impl::getTimeLord()->getTimeBase() * factor;
}

void
SST::Core::Serialization::serialize_impl<TimeConverter*>::operator()(
    TimeConverter*& s, SST::Core::Serialization::serializer& ser)
{
    SimTime_t factor = 0;

    switch ( ser.mode() ) {
    case serializer::SIZER:
    case serializer::PACK:
        // If s is nullptr, just put in a 0, otherwise get the factor
        if ( nullptr != s ) factor = s->getFactor();
        ser& factor;
        break;
    case serializer::UNPACK:
        ser& factor;

        // If we put in a nullptr, return a nullptr
        if ( factor == 0 ) {
            s = nullptr;
            return;
        }
        // Now get the TimeConverter for this factor.  Can only use
        // public APIs since there is no friend relationship with the
        // TimeLord.
        TimeLord*   timelord = Simulation_impl::getTimeLord();
        UnitAlgebra base     = timelord->getTimeBase();
        base *= factor;
        s = timelord->getTimeConverter(base);
        break;
    }
}
} // namespace SST
