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

#include "sst/core/output.h"
#include "sst/core/serialization/serializer.h"

#include <array>
#include <bitset>
#include <string>
#include <type_traits>
#include <typeinfo>
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
    void operator()(T& t, serializer& ser, ser_opt_t options)
    {
        using U          = std::remove_pointer_t<T>;
        const auto& tPtr = get_ptr(t);
        switch ( ser.mode() ) {
        case serializer::MAP:
            // Arithmetic, enum, complex types, and types constructible from std::string and convertible to std::string,
            // are handled in mapping mode without a custom serializer
            if constexpr ( std::is_arithmetic_v<U> || std::is_enum_v<U> || complex_properties<U>::is_complex ||
                           (std::is_constructible_v<U, std::string> && std::is_convertible_v<U, std::string>)) {
                ObjectMap* obj_map = new ObjectMapFundamental<U>(tPtr);
                if ( SerOption::is_set(options, SerOption::map_read_only) ) obj_map->setReadOnly();
                ser.mapper().map_object(ser.getMapName(), obj_map);
            }
            else {
                // Print warning at verbose levels >= 2, but only print it once for each type
                // This reduces surprise when a variable is not added to the ObjectMap
                static int UNUSED(once) = [] {
                    Output& output = Output::getDefaultObject();
                    if ( output.getVerboseLevel() >= 2 ) {
                        std::string typestr = ObjectMap::demangle_name(typeid(U).name());
                        const char* type    = typestr.c_str();
                        if constexpr ( std::is_class_v<U> )
                            output.verbose(CALL_INFO, 0, 0,
                                "Warning: Trivially serializable class type %s does not automatically have an "
                                "ObjectMap created for it.\nTo create an ObjectMap for %s, use one of these "
                                "methods:\n1. Add a serialize_order() method to %s.\n2. Define a serialize_impl<%s> "
                                "specialization.\n3. Add a constructor taking a std::string and an operator "
                                "std::string() const to %s, to allow conversion from/to std::string.\n\n",
                                type, type, type, type, type);
                        else if constexpr ( std::is_union_v<U> )
                            output.verbose(CALL_INFO, 0, 0,
                                "Warning: Trivially serializable union type %s does not automatically have an "
                                "ObjectMap created for it.\nTo create an ObjectMap for %s, use one of these "
                                "methods:\n1. Define a serialize_impl<%s> specialization.\n2. Add a constructor "
                                "taking a std::string and an operator std::string() const to %s, to allow "
                                "conversion from/to std::string.\n\n",
                                type, type, type, type);
                        else
                            output.verbose(CALL_INFO, 0, 0,
                                "Warning: Trivially serializable type %s does not automatically have an ObjectMap "
                                "created for it.\nTo create an ObjectMap for %s, define a serialize_impl<%s> "
                                "specialization.\n\n",
                                type, type, type);
                    }
                    return 0;
                }();
            }
            break;

        case serializer::UNPACK:
            if constexpr ( std::is_pointer_v<T> ) t = new U();
            [[fallthrough]];

        default:
            ser.primitive(*tPtr);
            break;
        }
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TRIVIAL_H
