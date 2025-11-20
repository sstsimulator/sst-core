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

namespace pvt {

// Proxy struct which represents a reference to std::atomic<T>
// This is only used in mapping mode
template <typename T>
class atomic_reference
{
    std::atomic<T>& ref;
    explicit atomic_reference(std::atomic<T>& ref) :
        ref(ref)
    {}

public:
    // Set the atomic reference to a value
    atomic_reference& operator=(const T& value)
    {
        ref.store(value);
        return *this;
    }

    // Convert the atomic reference to its value
    operator T() const { return ref.load(); }

    atomic_reference(const atomic_reference&)            = default;
    atomic_reference& operator=(const atomic_reference&) = delete;
    ~atomic_reference()                                  = default;

    friend class serialize_impl<std::atomic<T>>;
};

} // namespace pvt

template <class T>
class serialize_impl<std::atomic<T>>
{
    void operator()(std::atomic<T>& v, serializer& ser, ser_opt_t UNUSED(options))
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        {
            T t = v.load();
            SST_SER(t);
            // ser.size(t);
            break;
        }
        case serializer::PACK:
        {
            T t = v.load();
            SST_SER(t);
            break;
        }
        case serializer::UNPACK:
        {
            T val {};
            SST_SER(val);
            v.store(val);
            break;
        }
        case serializer::MAP:
        {
            // Create an ObjectMapReference referring to a pvt::atomic_reference<T> proxy wrapper class
            ser.mapper().map_hierarchy_start(ser.getMapName(),
                new ObjectMapReference<T, pvt::atomic_reference<T>, std::atomic<T>>(pvt::atomic_reference<T>(v)));
            ser.mapper().map_hierarchy_end();
            break;
        }
        }
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VECTOR_H
