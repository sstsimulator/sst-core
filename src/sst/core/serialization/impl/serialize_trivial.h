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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TRIVIAL_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TRIVIAL_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_trivial.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <array>
#include <bitset>
#include <type_traits>
#include <utility>

namespace SST::Core::Serialization {

// Types excluded because they would cause ambiguity with more specialized methods
template <typename T>
struct is_trivially_serializable_excluded : std::is_array<T>
{};

template <typename T, size_t S>
struct is_trivially_serializable_excluded<std::array<T, S>> : std::true_type
{};

template <size_t N>
struct is_trivially_serializable_excluded<std::bitset<N>> : std::true_type
{};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Version of serialize that works for trivially serializable types which aren't excluded, and pointers thereof //
//                                                                                                              //
// Note that the pointer tracking happens at a higher level, and only if it is turned on. If it is not turned   //
// on, then this only copies the value pointed to into the buffer. If multiple objects point to the same        //
// location, they will each have an independent copy after deserialization.                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
class serialize_impl<T,
    std::enable_if_t<std::conjunction_v<std::negation<is_trivially_serializable_excluded<std::remove_pointer_t<T>>>,
        is_trivially_serializable<std::remove_pointer_t<T>>>>>
{
    void operator()(T& t, serializer& ser, ser_opt_t UNUSED(options))
    {
        switch ( const auto mode = ser.mode() ) {
        case serializer::MAP:
            // Right now only arithmetic and enum types are handled in mapping mode without custom serializer
            if constexpr ( std::is_arithmetic_v<std::remove_pointer_t<T>> ||
                           std::is_enum_v<std::remove_pointer_t<T>> ) {
                ObjectMap* obj_map;
                if constexpr ( std::is_pointer_v<T> )
                    obj_map = new ObjectMapFundamental<std::remove_pointer_t<T>>(t);
                else
                    obj_map = new ObjectMapFundamental<T>(&t);
                if ( SerOption::is_set(options, SerOption::map_read_only) ) ser.mapper().setNextObjectReadOnly();
                ser.mapper().map_primitive(ser.getMapName(), obj_map);
            }
            else {
                // TODO: Handle mapping mode for trivially serializable types without from_string() methods which do not
                // define their own serialization methods.
            }
            break;

        default:
            if constexpr ( std::is_pointer_v<T> ) {
                if ( mode == serializer::UNPACK ) t = new std::remove_pointer_t<T> {};
                ser.primitive(*t);
            }
            else {
                ser.primitive(t);
            }
            break;
        }
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TRIVIAL_H
