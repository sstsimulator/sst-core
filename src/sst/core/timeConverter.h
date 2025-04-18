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

#ifndef SST_CORE_TIMECONVERTER_H
#define SST_CORE_TIMECONVERTER_H

#include "sst/core/serialization/serialize_impl_fwd.h"
#include "sst/core/sst_types.h"
#include "sst/core/unitAlgebra.h"

namespace SST {

class TimeLord;

/**
   A class to convert between a component's view of time and the
   core's view of time.
*/
class TimeConverter
{

    friend class TimeLord;

public:
    /**
       Converts from the component's view to the core's view of time.
       @param time time to convert to core time
     */
    SimTime_t convertToCoreTime(SimTime_t time) const { return time * factor; }

    /**
       Converts from the core's view to the components's view of time.
       The result is truncated, not rounded.
       @param time time to convert from core time
     */
    SimTime_t convertFromCoreTime(SimTime_t time) const { return time / factor; }

    /**
     * @return The factor used for conversions with Core Time
     */
    SimTime_t getFactor() const { return factor; }

    /**
       @return The period represented by this TimeConverter as a UnitAlgebra
     */
    UnitAlgebra getPeriod() const; // Implemented in timeLord.cc

private:
    /**
       Factor for converting between core and component time
    */
    SimTime_t factor;

    TimeConverter(SimTime_t fact) { factor = fact; }

    ~TimeConverter() {}

    TimeConverter() {} // Only needed to simplify serialization
};

template <>
class SST::Core::Serialization::serialize_impl<TimeConverter*>
{
    template <class A>
    friend class serialize;
    // Function implemented in timeLord.cc
    void operator()(TimeConverter*& s, serializer& ser);
};

} // namespace SST

#endif // SST_CORE_TIMECONVERTER_H
