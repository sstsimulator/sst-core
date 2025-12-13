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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TUPLE_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TUPLE_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_tuple.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace SST::Core::Serialization {

// Serialize tuples and pairs
template <typename T>
class serialize_impl<T,
    std::enable_if_t<is_same_type_template_v<T, std::tuple> || is_same_type_template_v<T, std::pair>>>
{
    template <typename>
    struct serialize_tuple;

    // For a sequence of indices, serialize each element of the std::tuple
    // This is only used in mapping mode
    template <size_t... INDEX>
    struct serialize_tuple<std::index_sequence<INDEX...>>
    {
        void operator()(T& t, serializer& ser, ser_opt_t opt)
        {
            (SST_SER_NAME(std::get<INDEX>(t), std::to_string(INDEX).c_str(), opt), ...);
        }
    };

    void operator()(T& t, serializer& ser, ser_opt_t options)
    {
        ser_opt_t opt = SerOption::is_set(options, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;
        switch ( ser.mode() ) {
        case serializer::MAP:
            ser.mapper().map_hierarchy_start(ser.getMapName(), new ObjectMapContainer<T>(&t));
            if constexpr ( is_same_type_template_v<T, std::pair> ) {
                // Serialize first and second members of std::pair
                SST_SER_NAME(t.first, "first", opt);
                SST_SER_NAME(t.second, "second", opt);
            }
            else {
                // Serialize each element in a std::tuple
                serialize_tuple<std::make_index_sequence<std::tuple_size_v<T>>>()(t, ser, opt);
            }
            ser.mapper().map_hierarchy_end();
            break;

        default:
            // Serialize each element in a std::tuple or std::pair
            std::apply([&](auto&... e) { ((SST_SER(e, opt)), ...); }, t);
            break;
        }
    }

    SST_FRIEND_SERIALIZE();
};

// Serialize pointers to tuple and pairs
template <typename T>
class serialize_impl<T*,
    std::enable_if_t<is_same_type_template_v<T, std::tuple> || is_same_type_template_v<T, std::pair>>>
{
    void operator()(T*& obj, serializer& ser, ser_opt_t options)
    {
        if ( ser.mode() == serializer::UNPACK ) obj = new T;
        SST_SER(*obj, options);
    }
    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TUPLE_H
