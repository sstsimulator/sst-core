// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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
#include <typeinfo>

namespace SST {

using Core::ThreadSafe::Spinlock;

TimeConverter*
TimeLord::getTimeConverter(const std::string& ts)
{
    // See if this is in the cache
    std::lock_guard<std::recursive_mutex> lock(slock_);
    if ( parseCache_.find(ts) == parseCache_.end() ) {
        TimeConverter* tc = getTimeConverter(UnitAlgebra(ts));
        parseCache_[ts]   = tc;
        return tc;
    }
    return parseCache_[ts];
}

TimeConverter*
TimeLord::getTimeConverter(SimTime_t simCycles)
{
    // Check to see if we already have a TimeConverter with this value
    std::lock_guard<std::recursive_mutex> lock(slock_);
    if ( tcMap_.find(simCycles) == tcMap_.end() ) {
        TimeConverter* tc = new TimeConverter(simCycles);
        tcMap_[simCycles] = tc;
        return tc;
    }
    return tcMap_[simCycles];
}

TimeConverter*
TimeLord::getTimeConverter(const UnitAlgebra& ts)
{
    Output& abort = Output::getDefaultObject();
    if ( !initialized_ ) {
        abort.fatal(CALL_INFO, 1, "Time Lord has not yet been initialized!");
    }
    SimTime_t      simCycles = getFactorForTime(ts);
    TimeConverter* tc        = getTimeConverter(simCycles);
    return tc;
}

void
TimeLord::init(const std::string& _timeBaseString)
{
    initialized_    = true;
    timeBaseString_ = _timeBaseString;
    timeBase_       = UnitAlgebra(timeBaseString_);

    try {
        nano_ = getTimeConverter("1ns");
    }
    catch ( const std::underflow_error& e ) {
        // This means that the core timebase is too big to represent
        // this time. Just set it to nulllptr
        nano_ = nullptr;
    }

    try {
        micro_ = getTimeConverter("1us");
    }
    catch ( const std::underflow_error& e ) {
        // This means that the core timebase is too big to represent
        // this time. Just set it to nulllptr
        micro_ = nullptr;
    }

    try {
        milli_ = getTimeConverter("1ms");
    }
    catch ( const std::underflow_error& e ) {
        // This means that the core timebase is too big to represent
        // this time. Just set it to nulllptr
        milli_ = nullptr;
    }
}

TimeLord::~TimeLord()
{
    // Delete all the TimeConverter objects
    std::map<ComponentId_t, LinkMap*>::iterator it;
    for ( TimeConverterMap_t::iterator it = tcMap_.begin(); it != tcMap_.end(); ++it ) {
        delete it->second;
    }
    tcMap_.clear();

    // Clear the contents of the cache
    parseCache_.clear();
}

SimTime_t
TimeLord::getSimCycles(const std::string& ts, const std::string& UNUSED(where))
{
    // See if this is in the cache
    std::lock_guard<std::recursive_mutex> lock(slock_);
    if ( parseCache_.find(ts) == parseCache_.end() ) {
        TimeConverter* tc = getTimeConverter(UnitAlgebra(ts));
        parseCache_[ts]   = tc;
        return tc->getFactor();
    }
    return parseCache_[ts]->getFactor();
}

UnitAlgebra
TimeConverter::getPeriod() const
{
    return Simulation_impl::getTimeLord()->getTimeBase() * factor;
}

SimTime_t
TimeLord::getFactorForTime(const std::string& time)
{
    // TODO: Once support for TimeConvert* is removed, this function can be cleaned up

    // See if this is in the cache
    std::scoped_lock<std::recursive_mutex> lock(slock_);
    if ( parseCache_.find(time) == parseCache_.end() ) {
        TimeConverter* tc = getTimeConverter(UnitAlgebra(time));
        parseCache_[time] = tc;
        return tc->factor;
    }
    return parseCache_[time]->factor;
}

SimTime_t
TimeLord::getFactorForTime(const UnitAlgebra& time)
{
    SimTime_t   simCycles;
    UnitAlgebra period = time;
    UnitAlgebra uaFactor;
    // Need to differentiate between Hz and s.
    if ( period.hasUnits("s") ) {
        uaFactor = period / timeBase_;
    }
    else if ( period.hasUnits("Hz") ) {
        UnitAlgebra temp = timeBase_;
        uaFactor         = temp.invert() / period;
    }
    else {
        throw std::invalid_argument("Error:  TimeConverter creation requires "
                                    "a time unit (s or Hz), " +
                                    time.toStringBestSI() + " was passed to call");
    }
    // Check to see if number is too big or too small
    if ( uaFactor.getValue() > static_cast<uint64_t>(MAX_SIMTIME_T) ) {
        throw std::overflow_error("Error:  Attempting to get TimeConverter for a time (" + time.toStringBestSI() +
                                  ") "
                                  "which is too large for the timebase (" +
                                  timeBase_.toStringBestSI() + ")");
    }
    // Check to see if period is too short (0 is special cased to not fail)
    if ( uaFactor.getValue() < 1 && uaFactor.getValue() != 0 ) {
        throw std::underflow_error("Error:  Attempting to get TimeConverter for a time (" + time.toStringBestSI() +
                                   ") "
                                   "which has too small of a period to be represented by the timebase (" +
                                   timeBase_.toStringBestSI() + ")");
    }
    simCycles = uaFactor.getRoundedValue();
    return simCycles;
}


TimeConverter::TimeConverter(const std::string& time)
{
    factor = Simulation_impl::getSimulation()->timeLord.getFactorForTime(time);
}

TimeConverter::TimeConverter(const UnitAlgebra& time)
{
    factor = Simulation_impl::getSimulation()->timeLord.getFactorForTime(time);
}

namespace Core::Serialization {

template <>
class ObjectMapFundamental<TimeConverter*> : public ObjectMap
{
protected:
    /**
       Address of the variable for reading and writing
     */
    TimeConverter** addr_ = nullptr;

public:
    // We'll treat this as a period when printing
    std::string get() const final override
    {
        if ( nullptr == *addr_ ) return "nullptr";
        TimeLord*   timelord = Simulation_impl::getTimeLord();
        UnitAlgebra base     = timelord->getTimeBase();
        base *= (*addr_)->getFactor();
        return base.toStringBestSI();
    }

    void set_impl(const std::string& UNUSED(value)) final override { return; }

    // We'll act like we're a fundamental type
    bool isFundamental() const final override { return true; }

    /**
       Get the address of the variable represented by the ObjectMap

       @return Address of varaible
     */
    void* getAddr() const final override { return addr_; }

    explicit ObjectMapFundamental(TimeConverter** addr) :
        ObjectMap(),
        addr_(addr)
    {
        setReadOnly(true);
    }

    std::string getType() const override { return demangle_name(typeid(TimeConverter).name()); }
};

template <>
class ObjectMapFundamental<TimeConverter> : public ObjectMap
{
protected:
    /**
       Address of the variable for reading and writing
     */
    TimeConverter* addr_ = nullptr;

public:
    // We'll treat this as a period when printing
    std::string get() const final override
    {
        TimeLord*   timelord = Simulation_impl::getTimeLord();
        UnitAlgebra base     = timelord->getTimeBase();
        base *= addr_->getFactor();
        return base.toStringBestSI();
    }

    void set_impl(const std::string& UNUSED(value)) final override { return; }

    // We'll act like we're a fundamental type
    bool isFundamental() const final override { return true; }

    /**
       Get the address of the variable represented by the ObjectMap

       @return Address of varaible
     */
    void* getAddr() const final override { return addr_; }

    explicit ObjectMapFundamental(TimeConverter* addr) :
        ObjectMap(),
        addr_(addr)
    {
        setReadOnly(true);
    }

    std::string getType() const override { return demangle_name(typeid(TimeConverter).name()); }
};

void
serialize_impl<TimeConverter>::operator()(TimeConverter& s, serializer& ser, ser_opt_t options)
{
    switch ( ser.mode() ) {
    case serializer::SIZER:
    case serializer::PACK:
    case serializer::UNPACK:
        SST_SER(s.factor);
        break;
    case serializer::MAP:
    {
        ObjectMap* obj_map = new ObjectMapFundamental<TimeConverter>(&s);
        if ( SerOption::is_set(options, SerOption::map_read_only) ) obj_map->setReadOnly();
        ser.mapper().map_object(ser.getMapName(), obj_map);
        break;
    }
    }
}


void
serialize_impl<TimeConverter*>::operator()(TimeConverter*& s, serializer& ser, ser_opt_t options)
{
    SimTime_t factor = 0;

    switch ( ser.mode() ) {
    case serializer::SIZER:
    case serializer::PACK:
        // If s is nullptr, just put in a 0, otherwise get the factor
        if ( nullptr != s ) factor = s->getFactor();
        SST_SER(factor);
        break;
    case serializer::UNPACK:
    {
        SST_SER(factor);

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
    case serializer::MAP:
    {
        ObjectMap* obj_map = new ObjectMapFundamental<TimeConverter*>(&s);
        if ( SerOption::is_set(options, SerOption::map_read_only) ) obj_map->setReadOnly();
        ser.mapper().map_object(ser.getMapName(), obj_map);
        break;
    }
    }
}

} // namespace Core::Serialization

} // namespace SST
