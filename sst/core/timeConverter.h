// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _SST_TIMECONVERTER_H
#define _SST_TIMECONVERTER_H

#include <sst/sst.h>
#include <sst/boost.h>

namespace SST {

class TimeLord;

/**
   A class to convert between a component's view of time and the
   core's view of time.
*/
class TimeConverter {

    friend class TimeLord;
  
 public:
    /**
       Converts from the component's view to the core's view of time.
       \param time time to convert to core time
     */
    SimTime_t convertToCoreTime(SimTime_t time) {
	return time * factor;
    }
    
    /**
       Converts from the core's view to the components's view of time.
       The result is truncated, not rounded.
       \param time time to convert from core time
     */
    SimTime_t convertFromCoreTime(SimTime_t time) {
	return time/factor;
    }

    SimTime_t getFactor() {
	return factor;
    }    
    
 private:
    /**
       Factor for converting between core and component time
    */
    SimTime_t factor;
    
    TimeConverter(SimTime_t fact) {
	factor = fact;
    }

    ~TimeConverter() {
    }
    

    // Boost serialization from here down
    TimeConverter() {}   // Only needed to simplify serialization

#if WANT_CHECKPOINT_SUPPORT

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version ) {
	printf("TimeConverter::serialize()\n");
	ar & BOOST_SERIALIZATION_NVP(factor);
    }
    
#endif
    
};    

} // namespace SST

#endif
