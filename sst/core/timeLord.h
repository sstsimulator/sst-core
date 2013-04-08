// -*- c++ -*-

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


#ifndef _SST_TIMELORD_H
#define _SST_TIMELORD_H

#include <string>
#include <map>

#include <sst/core/sst_types.h>
#include <sst/core/simulation.h>

namespace SST {

class TimeConverter;

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
    SimTime_t getSimCycles(std::string timeString, std::string where);

    TimeConverter* getNano() {return nano;}
    TimeConverter* getMicro() {return micro;}
    TimeConverter* getMilli() {return milli;}
    /* Used by power API to calculate time period in sec since last power query*/
    Time_t getSecFactor(){ return (Time_t)sec_factor;}
    
 private:
    friend class SST::SimulationBase;

    // Needed by the simulator to turn minPart back into a
    // TimeConverter object.
    TimeConverter *getTimeConverter(SimTime_t simCycles);

    TimeLord(std::string timeBaseString);
    ~TimeLord();

    TimeLord() { }                      // For serialization
    TimeLord(TimeLord const&);          // Don't Implement
    void operator=(TimeLord const&);    // Don't Implement
    
    // Variables that need to be saved when serialized
    std::string timeBaseString;
    TimeConverterMap_t tcMap;

    // Variables that will be recreated so don't need to be saved when
    // serialized
    double sec_factor;
    StringToTCMap_t parseCache;

    TimeConverter* nano;
    TimeConverter* micro;
    TimeConverter* milli;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_NVP(timeBaseString);
        ar & BOOST_SERIALIZATION_NVP(tcMap);
        ar & BOOST_SERIALIZATION_NVP(sec_factor);
        ar & BOOST_SERIALIZATION_NVP(parseCache);
        ar & BOOST_SERIALIZATION_NVP(nano);
        ar & BOOST_SERIALIZATION_NVP(micro);
        ar & BOOST_SERIALIZATION_NVP(milli);
    }
};    

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::TimeLord)

#endif
