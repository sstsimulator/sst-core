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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UTILITY_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UTILITY_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_utility.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include <iterator>
#include <type_traits>
#include <utility>

namespace SST::Core::Serialization {

// Whether two names are the same template. Similar to std::is_same.
template <template <typename...> class, template <typename...> class>
struct is_same_template : std::false_type
{};

template <template <typename...> class T>
struct is_same_template<T, T> : std::true_type
{};

template <template <typename...> class A, template <typename...> class B>
inline constexpr bool is_same_template_v = is_same_template<A, B>::value;

// Insert an element if a push_back() method with a single argument exists
template <typename T, typename V>
decltype(std::declval<T>().push_back(std::declval<V>()))
insert_element(T&& t, V&& v)
{
    return std::forward<T>(t).push_back(std::forward<V>(v));
}

// Insert an element if an insert() method with a single argument exists
template <typename T, typename V>
decltype(std::declval<T>().insert(std::declval<V>()))
insert_element(T&& t, V&& v)
{
    return std::forward<T>(t).insert(std::forward<V>(v));
}

// Insert an element if an insert_after() method exists
// Third parameter is insert_after iterator, defaulted for SFINAE matching
template <typename T, typename V>
decltype(std::declval<T>().insert_after(std::declval<T>().before_begin(), std::declval<V>()))
insert_element(T&& t, V&& v, decltype(std::declval<T>().before_begin()) after = {})
{
    return std::forward<T>(t).insert_after(after, std::forward<V>(v));
}

// Whether a size() method exists for a type
template <typename, typename = void>
inline constexpr bool has_size_v = false;

template <typename T>
inline constexpr bool has_size_v<T, std::void_t<decltype(std::declval<T>().size())>> = true;

// Whether std::distance(begin(), end()) exists for a type
template <typename, typename = void>
inline constexpr bool has_distance_v = false;

template <typename T>
inline constexpr bool
    has_distance_v<T, std::void_t<decltype(std::distance(std::declval<T>().begin(), std::declval<T>().end()))>> = true;

// Get the size of a container, using a size() method if it exists, otherwise distance(begin, end)
template <typename T>
std::enable_if_t<has_size_v<T> || has_distance_v<T>, size_t>
get_size(const T& t)
{
    if constexpr ( has_size_v<T> )
        return t.size();
    else
        return std::distance(t.begin(), t.end());
}

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UTILITY_H
