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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_AGGREGATE_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_AGGREGATE_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_aggregate.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/impl/serialize_utility.h"
#include "sst/core/serialization/serializer.h"

#include <array>
#include <cstddef>
#include <type_traits>

namespace SST::Core::Serialization {

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Aggregate serialization                                                                         //
//                                                                                                 //
// An aggregate is a C-style array, std::array, or a class/struct/union with all public non-static //
// data members and direct bases, no user-provided, inherited or explicit constructors (C++17), no //
// user-declared or inherited constructors (C++20), and no virtual functions or virtual bases.     //
//                                                                                                 //
// For aggregates which are either not trivially copyable or are not standard layout types, they   //
// can be serialized by using structured bindings to extract their members and serialize each      //
// member separately. The serialization implementation needs to be specialized for each member     //
// count, because until C++26, structured binding packs are not supported.                         //
//                                                                                                 //
// For aggregate serialization,                                                                    //
// - It must be an aggregate class type                                                            //
// - It must not be trivially serializable                                                         //
// - It must not have a serialize_order() method                                                   //
// - It must not contain anonymous union members (not detectable in C++17 except by error)         //
// - The number of fields must be supported by the implementation below                            //
/////////////////////////////////////////////////////////////////////////////////////////////////////

namespace pvt {

template <typename T, size_t NFIELDS>
struct serialize_aggregate_impl : std::false_type
{};

// Structured binding extracts NFIELDS fields in __VA_ARGS__ and serializes each one separately
#define SERIALIZE_AGGREGATE_IMPL(NFIELDS, ...)                        \
    template <typename T>                                             \
    struct serialize_aggregate_impl<T, NFIELDS> : std::true_type      \
    {                                                                 \
        void operator()(T& t, serializer& ser, ser_opt_t UNUSED(opt)) \
        {                                                             \
            auto& [__VA_ARGS__] = t;                                  \
            [&](auto&... e) { (SST_SER(e), ...); }(__VA_ARGS__);      \
        }                                                             \
    }

SERIALIZE_AGGREGATE_IMPL(1, a0);
SERIALIZE_AGGREGATE_IMPL(2, a0, a1);
SERIALIZE_AGGREGATE_IMPL(3, a0, a1, a2);
SERIALIZE_AGGREGATE_IMPL(4, a0, a1, a2, a3);
SERIALIZE_AGGREGATE_IMPL(5, a0, a1, a2, a3, a4);
SERIALIZE_AGGREGATE_IMPL(6, a0, a1, a2, a3, a4, a5);
SERIALIZE_AGGREGATE_IMPL(7, a0, a1, a2, a3, a4, a5, a6);
SERIALIZE_AGGREGATE_IMPL(8, a0, a1, a2, a3, a4, a5, a6, a7);
SERIALIZE_AGGREGATE_IMPL(9, a0, a1, a2, a3, a4, a5, a6, a7, a8);
SERIALIZE_AGGREGATE_IMPL(10, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
SERIALIZE_AGGREGATE_IMPL(11, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
SERIALIZE_AGGREGATE_IMPL(12, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
SERIALIZE_AGGREGATE_IMPL(13, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
SERIALIZE_AGGREGATE_IMPL(14, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
SERIALIZE_AGGREGATE_IMPL(15, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
SERIALIZE_AGGREGATE_IMPL(16, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
SERIALIZE_AGGREGATE_IMPL(17, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16);
SERIALIZE_AGGREGATE_IMPL(18, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17);
SERIALIZE_AGGREGATE_IMPL(19, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18);
SERIALIZE_AGGREGATE_IMPL(20, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19);

// An aggregate class type with no serialize_order() method, an implementation of serialize_aggregate_impl for the
// number of fields, and not trivially serializable
template <typename T>
constexpr bool is_aggregate_serializable_v =
    std::conjunction_v<std::is_class<T>, std::is_aggregate<T>, std::negation<has_serialize_order<T>>,
        serialize_aggregate_impl<T, nfields<T>>, std::negation<is_trivially_serializable<T>>>;

// std::array is excluded because it is handled in serialize_array.h
template <typename T, size_t N>
constexpr bool is_aggregate_serializable_v<std::array<T, N>> = false;

} // namespace pvt

// Serialize aggregate class types which are not trivially serializable
template <typename T>
class serialize_impl<T, std::enable_if_t<pvt::is_aggregate_serializable_v<T>>> :
    pvt::serialize_aggregate_impl<T, nfields<T>>
{
    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_AGGREGATE_H
