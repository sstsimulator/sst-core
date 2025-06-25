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
    friend class SST::Core::Serialization::serialize_impl<TimeConverter>;
    friend class SST::Core::Serialization::serialize_impl<TimeConverter*>;

public:
    /**
       Create a new TimeConverter object from a TimeConverter*
       Use this to create a local TimeConverter from a TimeConverter*
       returned by the BaseComponent and other public APIs.
       @param tc TimeConverter to initialize factor from
     */
    TimeConverter(TimeConverter* tc) { factor = tc->factor; }

    [[deprecated("Use of shared TimeConverter objects is deprecated. If you're seeing this message, you likely have "
                 "changed a TimeConverter* to TimeConverter, but are still assigning it to be nullptr at the point of "
                 "this warning.")]]
    TimeConverter(std::nullptr_t UNUSED(tc))
    {
        factor = 0;
    }

    /**
       Do not directly invoke this constructor from Components to get
       a TimeConverter. Instead, use the BaseComponent API functions and the constructor
       that uses a TimeConverter* to create a TimeConverter.
     */
    TimeConverter() {}

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
       Resets a TimeConverter to uninitialized state (factor = 0)
     */
    void reset() { factor = 0; }

    /**
       @return The period represented by this TimeConverter as a UnitAlgebra
     */
    UnitAlgebra getPeriod() const; // Implemented in timeLord.cc

    /**
     * TimeConverter* returned by the core should *never* be deleted by
     * Elements. This was moved to public due to needing to support ObjectMaps.
     */
    ~TimeConverter() {}

    /**
       Function to check to see if the TimeConverter is initialized
       (non-zero factor)

       @return true if TimeConverter is initialized (factor is
       non-zero), false otherwise
     */
    bool isInitialized() const { return factor != 0; }

    /**
       Conversion to bool.  This will allow !tc to work to check if it
       has been initialized (has a non-zero factor).

       @return true if TimeConverter is initialized (factor is
       non-zero)
     */
    explicit operator bool() const { return factor != 0; }

private:
    /**
       Factor for converting between core and component time
    */
    SimTime_t factor = 0;

    explicit TimeConverter(SimTime_t fact) { factor = fact; }
};

template <>
class SST::Core::Serialization::serialize_impl<TimeConverter>
{
    // Function implemented in timeLord.cc
    void operator()(TimeConverter& s, serializer& ser, ser_opt_t options);

    SST_FRIEND_SERIALIZE();
};

template <>
class SST::Core::Serialization::serialize_impl<TimeConverter*>
{
    // Function implemented in timeLord.cc
    void operator()(TimeConverter*& s, serializer& ser, ser_opt_t options);

    SST_FRIEND_SERIALIZE();
};

} // namespace SST

#endif // SST_CORE_TIMECONVERTER_H
