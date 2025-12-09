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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ADAPTER_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ADAPTER_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_adapter.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <queue>
#include <stack>
#include <type_traits>

namespace SST::Core::Serialization {

template <typename T>
constexpr bool is_adapter_v = is_same_type_template_v<T, std::stack> || is_same_type_template_v<T, std::queue> ||
                              is_same_type_template_v<T, std::priority_queue>;

// Serialize adapter classes std::stack, std::queue, std::priority_queue
template <typename T>
class serialize_impl<T, std::enable_if_t<is_adapter_v<T>>>
{
    struct S : T
    {
        using T::c; // access protected container
    };

    void operator()(T& v, serializer& ser, ser_opt_t options)
    {
        switch ( ser.mode() ) {
        case serializer::MAP:
            ser.mapper().map_hierarchy_start(ser.getMapName(), new ObjectMapContainer<T>(&v));

            // For std::priority_queue, mark the underlying container read-only
            if constexpr ( is_same_type_template_v<T, std::priority_queue> ) options |= SerOption::map_read_only;

            // serialize the underlying container
            SST_SER_NAME(static_cast<S&>(v).c, "container", options);

            ser.mapper().map_hierarchy_end();
            break;

        default:
            // serialize the underlying container
            SST_SER(static_cast<S&>(v).c, options);
            break;
        }
    }

    SST_FRIEND_SERIALIZE();
};

template <typename T>
class serialize_impl<T*, std::enable_if_t<is_adapter_v<T>>>
{
    void operator()(T*& obj, serializer& ser, ser_opt_t options)
    {
        if ( ser.mode() == serializer::UNPACK ) obj = new T;
        SST_SER(*obj, options);
    }
    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ADAPTER_H
