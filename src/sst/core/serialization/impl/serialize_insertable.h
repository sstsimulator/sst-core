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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_INSERTABLE_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_INSERTABLE_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_insertable.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/impl/serialize_utility.h"
#include "sst/core/serialization/serializer.h"

#include <cstddef>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
namespace SST::Core::Serialization {

// If the type is a pair with a const first, map it to pair with non-const first
template <typename T>
struct remove_const_key
{
    using type = T;
};

template <typename KEY, typename VALUE>
struct remove_const_key<std::pair<const KEY, VALUE>>
{
    using type = std::pair<KEY, VALUE>;
};

// Whether a size() method exists for a type
template <typename, typename = void>
constexpr bool has_size_v = false;

template <typename T>
constexpr bool has_size_v<T, std::void_t<decltype(std::declval<T>().size())>> = true;

// Whether std::distance(begin(), end()) exists for a type
template <typename, typename = void>
constexpr bool has_distance_v = false;

template <typename T>
constexpr bool
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

// Whether it is a std::vector<bool>
template <typename>
constexpr bool is_vector_bool_v = false;

template <typename... Ts>
constexpr bool is_vector_bool_v<std::vector<bool, Ts...>> = true;

// Whether it is a simple map (not a multimap and has integral, floating-point, enum, or convertible to string keys)
template <typename, typename = void>
constexpr bool is_simple_map_v = false;

template <template <typename...> class MAP, typename KEY, typename... REST>
constexpr bool is_simple_map_v<
    MAP<KEY, REST...>,
    std::enable_if_t<(is_same_template_v<MAP, std::map> || is_same_template_v<MAP, std::unordered_map>)&&(
        std::is_arithmetic_v<KEY> || std::is_enum_v<KEY> || std::is_convertible_v<KEY, std::string>)>> = true;

// Whether it is a simple set (not a multiset and has integral, floating-point, enum, or convertible to string keys)
template <typename, typename = void>
constexpr bool is_simple_set_v = false;

template <template <typename...> class SET, typename KEY, typename... REST>
constexpr bool is_simple_set_v<
    SET<KEY, REST...>,
    std::enable_if_t<(is_same_template_v<SET, std::set> || is_same_template_v<SET, std::unordered_set>)&&(
        std::is_arithmetic_v<KEY> || std::is_enum_v<KEY> || std::is_convertible_v<KEY, std::string>)>> = true;

// std::deque
// std::forward_list
// std::list
// std::map
// std::multimap
// std::multiset
// std::set
// std::unordered_map
// std::unordered_multimap
// std::unordered_multiset
// std::unordered_set
// std::vector, including std::vector<bool>
//
// clang-format off
template <template <typename...> class T, typename... Ts>
class serialize_impl<
    T<Ts...>, std::enable_if_t<
                  is_same_template_v< T, std::deque              > ||
                  is_same_template_v< T, std::forward_list       > ||
                  is_same_template_v< T, std::list               > ||
                  is_same_template_v< T, std::map                > ||
                  is_same_template_v< T, std::multimap           > ||
                  is_same_template_v< T, std::multiset           > ||
                  is_same_template_v< T, std::set                > ||
                  is_same_template_v< T, std::unordered_map      > ||
                  is_same_template_v< T, std::unordered_multimap > ||
                  is_same_template_v< T, std::unordered_multiset > ||
                  is_same_template_v< T, std::unordered_set      > ||
                  is_same_template_v< T, std::vector             >
                > >
// clang-format on
{
    // Object type = container template with template arguments
    using OBJ = T<Ts...>;

    // Value type of element with const removed from first of pair if it exists
    using value_type = typename remove_const_key<typename OBJ::value_type>::type;

    void operator()(OBJ& obj, serializer& ser, ser_opt_t options)
    {
        switch ( const auto mode = ser.mode() ) {
        case serializer::SIZER:
        case serializer::PACK:
        {
            size_t size = get_size(obj);

            if ( mode == serializer::PACK )
                ser.pack(size);
            else
                ser.size(size);

            if constexpr ( is_vector_bool_v<OBJ> ) {
                // For std::vector<bool>, iterate over bool values instead of references to elements.
                for ( bool e : obj )
                    // as_ptr_elem not valid for bool
                    SST_SER(e);
            }
            else {
                ser_opt_t opts =
                    SerOption::is_set(options, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;
                // Iterate over references to elements, casting away any const in keys
                for ( auto& e : obj )
                    SST_SER((value_type&) e, opts);
            }
            break;
        }

        case serializer::UNPACK:
        {
            // Get the total size of the container
            size_t size;
            ser.unpack(size);

            ser_opt_t opts = SerOption::is_set(options, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;

            // Erase the container
            obj.clear();
            if constexpr ( is_same_template_v<T, std::vector> ) obj.reserve(size); // Reserve size of vector

            if constexpr ( is_same_template_v<T, std::forward_list> ) {
                auto last = obj.before_begin(); // iterator of last element inserted
                for ( size_t i = 0; i < size; ++i ) {
                    last        = obj.emplace_after(last);
                    auto& value = *last;
                    SST_SER(value, opts);
                }
            }
            else {
                for ( size_t i = 0; i < size; ++i ) {
                    if constexpr ( is_same_template_v<T, std::map> || is_same_template_v<T, std::unordered_map> ) {
                        typename OBJ::key_type key {};
                        SST_SER(key);
                        auto& value = obj[std::move(key)];
                        SST_SER(value, opts);
                    }
                    else if constexpr (
                        is_same_template_v<T, std::multimap> || is_same_template_v<T, std::unordered_multimap> ) {
                        typename OBJ::key_type key {};
                        SST_SER(key);
                        auto& value = obj.emplace(std::move(key), typename OBJ::mapped_type {})->second;
                        SST_SER(value, opts);
                    }
                    else if constexpr (
                        is_same_template_v<T, std::set> || is_same_template_v<T, std::unordered_set> ||
                        is_same_template_v<T, std::multiset> || is_same_template_v<T, std::unordered_multiset> ) {
                        typename OBJ::key_type key {};
                        // TODO: Figure out how to make as_ptr_elem work with sets
                        SST_SER(key);
                        obj.emplace(std::move(key));
                    }
                    else if constexpr ( is_vector_bool_v<OBJ> ) {
                        bool value {};
                        SST_SER(value);
                        obj.push_back(value);
                    }
                    else { // std::vector, std::deque, std::list
                        auto& value = obj.emplace_back();
                        SST_SER(value, opts);
                    }
                }
            }
            // assert(size == get_size(obj));
            break;
        }

        case serializer::MAP:
        {
            using SST::Core::to_string;
            const std::string& name    = ser.getMapName();
            auto*              obj_map = new ObjectMapContainer<OBJ>(&obj);
            ser.mapper().map_hierarchy_start(name, obj_map);

            if constexpr ( is_vector_bool_v<OBJ> ) {
                // std::vector<bool>
                size_t i = 0;
                for ( bool e : obj )
                    sst_ser_object(ser, e, SerOption::none, to_string(i++).c_str());
            }
            else if constexpr ( is_simple_map_v<OBJ> ) {
                // non-multi maps with a simple key
                for ( auto& [key, value] : obj )
                    sst_ser_object(ser, value, SerOption::none, to_string(key).c_str());
            }
            // TODO: handle is_simple_set
            else {
                // std::vector, std::deque, std::list, std::forward_list, std::multimap,
                // std::unordered_multimap, std::multiset, std::unordered_multiset, and
                // std::map, std::set, std::unordered_map std::unordered_set with non-simple keys
                size_t i = 0;
                for ( auto& e : obj )
                    sst_ser_object(ser, (value_type&)e, SerOption::none, to_string(i++).c_str());
            }
            ser.mapper().map_hierarchy_end();
            break;
        }
        }
    }

    SST_FRIEND_SERIALZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_INSERTABLE_H
