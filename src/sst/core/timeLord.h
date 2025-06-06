// -*- c++ -*-

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

#ifndef SST_CORE_TIMELORD_H
#define SST_CORE_TIMELORD_H

#include "sst/core/sst_types.h"
#include "sst/core/threadsafe.h"
#include "sst/core/unitAlgebra.h"

#include <map>
#include <mutex>
#include <string>

extern int main(int argc, char** argv);

namespace SST {

class Link;
class Simulation;
class Simulation_impl;
class TimeConverter;
class UnitAlgebra;
class BaseComponent;

/**
    Class for creating and managing TimeConverter objects
 */
class TimeLord
{
    using TimeConverterMap_t = std::map<SimTime_t, TimeConverter*>;
    using StringToTCMap_t    = std::map<std::string, TimeConverter*>;

public:
    /**
        Create a new TimeConverter object using specified SI Units. For
        example, "1 Ghz" (1 Gigahertz), "2.5 ns" (2.5 nanoseconds).

        @param ts String indicating the base unit for this object. The
        string should be a floating point number followed by a prefix,
        and then frequency (i.e. Hz) or time unit (s). Allowable seconds
        prefixes are: 'f' (fempto), 'p' (pico), 'n' (nano), 'u' (micro),
        'm' (milli). Allowable frequency prefixes are 'k' (kilo), 'M'
        (mega), and 'G' (giga).
     */
    TimeConverter* getTimeConverter(const std::string& ts);
    /**
     * Create a new TimeConverter object using the specified units.
     *
     * @param ts UnitAlgebra object indicating the base unit for this object.
     */
    TimeConverter* getTimeConverter(const UnitAlgebra& ts);

    /**
     * @return Time Base of the TimeLord
     */
    UnitAlgebra getTimeBase() const { return timeBase; }

    /** @return TimeConverter which represents Nanoseconds */
    TimeConverter* getNano() { return nano; }
    /** @return TimeConverter which represents Microseconds */
    TimeConverter* getMicro() { return micro; }
    /** @return TimeConverter which represents Milliseconds */
    TimeConverter* getMilli() { return milli; }

    /** Not a Public API.
     * Returns the number of raw simulation cycles given by a specified time string
     */
    SimTime_t getSimCycles(const std::string& timeString, const std::string& where);

private:
    friend class SST::Simulation;
    friend class SST::Simulation_impl;
    friend class SST::Link;
    friend class SST::BaseComponent;

    friend int ::main(int argc, char** argv);

    void init(const std::string& timeBaseString);

    // Needed by the simulator to turn minPart back into a
    // TimeConverter object.
    TimeConverter* getTimeConverter(SimTime_t simCycles);

    TimeLord() :
        initialized(false)
    {}
    ~TimeLord();

    TimeLord(const TimeLord&)            = delete; // Don't Implement
    TimeLord& operator=(const TimeLord&) = delete; // Don't Implement

    bool                 initialized;
    std::recursive_mutex slock;

    std::string        timeBaseString;
    TimeConverterMap_t tcMap;
    UnitAlgebra        timeBase;

    // double sec_factor;
    StringToTCMap_t parseCache;

    TimeConverter* nano;
    TimeConverter* micro;
    TimeConverter* milli;
};

} // namespace SST

#endif // SST_CORE_TIMELORD_H
