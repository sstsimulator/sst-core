// -*- c++ -*-

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_TIMELORD_H
#define SST_CORE_TIMELORD_H

#include <sst/core/sst_types.h>

#include <map>
#include <string>

#include <sst/core/simulation.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/threadsafe.h>

extern int main(int argc, char **argv);

namespace SST {

class TimeConverter;
class UnitAlgebra;
    
  /** 
      Class for creating and managing TimeConverter objects
   */
class TimeLord {
    typedef std::map<SimTime_t,TimeConverter*> TimeConverterMap_t;
    typedef std::map<std::string,TimeConverter*> StringToTCMap_t;
    
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
    TimeConverter* getTimeConverter(std::string ts);
    /**
     * Create a new TimeConverter object using the specified units.
     *
     * @param ts UnitAlgebra object indicating the base unit for this object.
     */
    TimeConverter* getTimeConverter(const UnitAlgebra& ts);

    /** Not a Public API.
     * Returns the number of raw simulation cycles given by a specified time string
     */
    SimTime_t getSimCycles(std::string timeString, std::string where);
    /**
     * Return the Time Base of the TimeLord
     */
    UnitAlgebra getTimeBase() const { return timeBase; }

    /** Return a TimeConverter which represents Nanoseconds */
    TimeConverter* getNano() {return nano;}
    /** Return a TimeConverter which represents Microseconds */
    TimeConverter* getMicro() {return micro;}
    /** Return a TimeConverter which represents Milliseconds */
    TimeConverter* getMilli() {return milli;}

 private:
    friend class SST::Simulation;
    friend int ::main(int argc, char **argv);

    void init(std::string timeBaseString);

    // Needed by the simulator to turn minPart back into a
    // TimeConverter object.
    TimeConverter *getTimeConverter(SimTime_t simCycles);

    TimeLord() : initialized(false) { }
    ~TimeLord();

    TimeLord(TimeLord const&);          // Don't Implement
    void operator=(TimeLord const&);    // Don't Implement

    bool initialized;
    std::recursive_mutex slock;

    // Variables that need to be saved when serialized
    std::string timeBaseString;
    TimeConverterMap_t tcMap;
    UnitAlgebra timeBase;

    // double sec_factor;
    StringToTCMap_t parseCache;

    TimeConverter* nano;
    TimeConverter* micro;
    TimeConverter* milli;

};    

} // namespace SST

#endif //SST_CORE_TIMELORD_H
