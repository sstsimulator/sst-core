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

#include <forward_list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SST::Core::Serialization {

// A type with begin(), end() methods with value_type elements which can call insert_element()
// to insert them in begin() to end() order
//
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
// and any user-defined or future STL classes which have begin() and end() methods, a
// value_type element type, and an insert_element() overload which can append value_type

template <class T>
class serialize_impl<
    T, std::void_t<
           // exclude std::string and std::string* to prevent specialization ambiguity
           std::enable_if_t<!std::is_same_v<std::remove_pointer_t<T>, std::string>>,

           // whether begin() method exists
           decltype(std::declval<T>().begin()),

           // whether end() method exists
           decltype(std::declval<T>().end()),

           // whether a clear() method exists
           decltype(std::declval<T>().clear()),

           // whether get_size() can determine size because it has a matching overload
           decltype(get_size(std::declval<T>())),

           // whether insert_element() can insert value_type elements
           decltype(insert_element(std::declval<T>(), std::declval<typename T::value_type>()))>>
{
    // If the type is a pair with a const first, map it to pair with non-const first
    template <typename U>
    struct remove_const_key
    {
        using type = U;
    };

    template <typename KEY, typename VALUE>
    struct remove_const_key<std::pair<const KEY, VALUE>>
    {
        using type = std::pair<KEY, VALUE>;
    };

    // Value type of element, with const removed from first of pair if it exists
    using value_type = typename remove_const_key<typename T::value_type>::type;

    // Note: the following use struct templates because of a GCC bug which does
    // not allow static constexpr variable templates defined inside of a class.

    // Whether it is a std::vector
    template <typename>
    struct is_vector : std::false_type
    {};

    template <typename... Ts>
    struct is_vector<std::vector<Ts...>> : std::true_type
    {};

    // Whether it is a std::vector<bool>
    template <typename>
    struct is_vector_bool : std::false_type
    {};

    template <typename... Ts>
    struct is_vector_bool<std::vector<bool, Ts...>> : std::true_type
    {};

    // Whether it is a std::forward_list
    template <typename>
    struct is_forward_list : std::false_type
    {};

    template <typename... Ts>
    struct is_forward_list<std::forward_list<Ts...>> : std::true_type
    {};

    // Whether it is a simple map (not a multimap and has integral, floating-point, enum, or convertible to string keys)
    template <typename, typename = void>
    struct is_simple_map : std::false_type
    {};

    template <template <typename...> class MAP, typename KEY, typename... REST>
    struct is_simple_map<
        MAP<KEY, REST...>,
        std::enable_if_t<(is_same_template_v<MAP, std::map> || is_same_template_v<MAP, std::unordered_map>)&&(
            std::is_arithmetic_v<KEY> || std::is_enum_v<KEY> || std::is_convertible_v<KEY, std::string>)>> :
        std::true_type
    {};

    // Whether it is a simple set (not a multiset and has integral, floating-point, enum, or convertible to string keys)
    template <typename, typename = void>
    struct is_simple_set : std::false_type
    {};

    template <template <typename...> class SET, typename KEY, typename... REST>
    struct is_simple_set<
        SET<KEY, REST...>,
        std::enable_if_t<(is_same_template_v<SET, std::set> || is_same_template_v<SET, std::unordered_set>)&&(
            std::is_arithmetic_v<KEY> || std::is_enum_v<KEY> || std::is_convertible_v<KEY, std::string>)>> :
        std::true_type
    {};

public:
    void operator()(T& t, serializer& ser)
    {
        switch ( const auto mode = ser.mode() ) {
        case serializer::SIZER:
        case serializer::PACK:
        {
            size_t size = get_size(t);

            if ( mode == serializer::PACK )
                ser.pack(size);
            else
                ser.size(size);

            if constexpr ( is_vector_bool<T>::value ) {
                // For std::vector<bool>, iterate over bool values instead of references to elements
                for ( bool e : t )
                    ser& e;
            }
            else {
                // Iterate over references to elements, casting away any const in keys
                for ( auto& e : t )
                    ser&(value_type&)e;
            }
            break;
        }

        case serializer::UNPACK:
        {
            size_t size;
            ser.unpack(size);

            t.clear();                                            // Clear the container
            if constexpr ( is_vector<T>::value ) t.reserve(size); // Reserve size of vector

            // For std::forward_list, last is iterator of last element inserted
            decltype(t.begin()) last [[maybe_unused]];
            if constexpr ( is_forward_list<T>::value ) last = t.before_begin();

            for ( size_t i = 0; i < size; ++i ) {
                value_type e {}; // For now, elements have to be default-initializable
                ser&       e;    // Unpack the element

                // Insert the element, moving it for efficiency
                if constexpr ( is_forward_list<T>::value )
                    last = insert_element(t, std::move(e), last);
                else
                    insert_element(t, std::move(e));
            }
            // assert(size == get_size(t));
            break;
        }

        case serializer::MAP:
        {
            using SST::Core::to_string;
            const std::string& name    = ser.getMapName();
            auto*              obj_map = new ObjectMapContainer<T>(&t);
            ser.mapper().map_hierarchy_start(name, obj_map);

            if constexpr ( is_vector_bool<T>::value ) {
                // std::vector<bool>
                size_t i = 0;
                for ( bool e : t )
                    sst_map_object(ser, e, to_string(i++));
            }
            else if constexpr ( is_simple_map<T>::value ) {
                // non-multi maps with a simple key
                for ( auto& [key, value] : t )
                    sst_map_object(ser, value, to_string(key));
            }
            // TODO: handle is_simple_set
            else {
                // std::vector, std::deque, std::list, std::forward_list, std::multimap,
                // std::unordered_multimap, std::multiset, std::unordered_multiset, and
                // std::map, std::set, std::unordered_map std::unordered_set with non-simple keys
                size_t i = 0;
                for ( auto& e : t )
                    sst_map_object(ser, (value_type&)e, to_string(i++));
            }
            ser.mapper().map_hierarchy_end();
            break;
        }
        }
    }
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_INSERTABLE_H
