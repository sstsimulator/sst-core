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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ATOMIC_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ATOMIC_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_atomic.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <atomic>

namespace SST::Core::Serialization {

template <class T>
class serialize_impl<std::atomic<T>>
{
    // Proxy class which represents a reference to std::atomic<T>, is copyable, convertible to T, and assignable from T
    //
    // This is only used in mapping mode
    class atomic_reference
    {
        std::atomic<T>& ref;

    public:
        explicit atomic_reference(std::atomic<T>& ref) :
            ref(ref)
        {}

        // Set the referenced atomic to a value
        atomic_reference& operator=(const T& value)
        {
            ref.store(value);
            return *this;
        }

        // Convert the referenced atomic to its value
        operator T() const { return ref.load(); }
    };

    void operator()(std::atomic<T>& v, serializer& ser, ser_opt_t UNUSED(options))
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        case serializer::PACK:
        {
            T value { v.load() };
            SST_SER(value);
            break;
        }
        case serializer::UNPACK:
        {
            T value {};
            SST_SER(value);
            v.store(value);
            break;
        }
        case serializer::MAP:
        {
            // Create an ObjectMapFundamentalReference referring to a atomic_reference proxy wrapper class
            ser.mapper().map_hierarchy_start(ser.getMapName(),
                new ObjectMapFundamentalReference<T, atomic_reference, std::atomic<T>>(atomic_reference(v)));
            ser.mapper().map_hierarchy_end();
            break;
        }
        }
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VECTOR_H
