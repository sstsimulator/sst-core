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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ARRAY_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ARRAY_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_array.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <array>
#include <cstddef>
#include <string>
#include <type_traits>

namespace SST::Core::Serialization {

// Serialize arrays

namespace pvt {

// Wrapper classes. They have no declared constructors so that they can be aggregate-initialized.
template <typename ELEM_T, typename SIZE_T>
struct array_wrapper
{
    ELEM_T*& ptr;
    SIZE_T&  size;
};

template <typename ELEM_T>
struct raw_ptr_wrapper
{
    ELEM_T*& ptr;
};

// Functions for serializing arrays element by element
void serialize_array(
    serializer& ser, void* data, size_t size, void serialize_array_element(serializer& ser, void* data, size_t index));

void serialize_array_map(
    serializer& ser, void* data, size_t size, ObjectMap* map,
    void serialize_array_map_element(serializer& ser, void* data, size_t index, const std::string& name));

// Serialize an array element
// Separated out to reduce code size
template <typename ELEM_T>
void
serialize_array_element(serializer& ser, void* data, size_t index)
{
    ser& static_cast<ELEM_T*>(data)[index];
}

// Return a new map representing canonical fixed sized array (whether the original array came from ELEM_T[SIZE] or
// std::array<ELEM_T, SIZE>)
template <typename ELEM_T, size_t SIZE>
ObjectMap*
new_fixed_array_map(void* data)
{
    return new ObjectMapContainer<ELEM_T[SIZE]>(static_cast<ELEM_T(*)[SIZE]>(data));
}

// Serialize an array map element
// Separated out to reduce code size
template <typename ELEM_T>
void
serialize_array_map_element(serializer& ser, void* data, size_t index, const std::string& name)
{
    sst_map_object(ser, static_cast<ELEM_T*>(data)[index], name);
}

// Whether the element type is copyable with memcpy()
// TODO: Implement with std::is_trivially_copyable and std::is_aggregate, using reflection to check for troublesome
// members like pointers
template <typename T>
constexpr bool is_trivial_element_v = std::is_arithmetic_v<T> || std::is_enum_v<T>;

// Serialize fixed arrays
template <typename OBJ_TYPE, typename ELEM_T, size_t SIZE>
struct serialize_impl_fixed_array
{
    void operator()(OBJ_TYPE& ary, serializer& ser)
    {
        const auto& aPtr = get_ptr(ary);
        switch ( ser.mode() ) {
        case serializer::MAP:
            serialize_array_map(
                ser, &(*aPtr)[0], SIZE, new_fixed_array_map<ELEM_T, SIZE>(&(*aPtr)[0]),
                serialize_array_map_element<ELEM_T>);
            break;

        case serializer::UNPACK:
            if constexpr ( std::is_pointer_v<OBJ_TYPE> ) {
                if constexpr ( std::is_same_v<OBJ_TYPE, ELEM_T(*)[SIZE]> )
                    ary = new ELEM_T[SIZE];
                else
                    ary = new std::remove_pointer_t<OBJ_TYPE>;
            }
            [[fallthrough]];

        default:
            if constexpr ( is_trivial_element_v<ELEM_T> )
                ser.raw(&(*aPtr)[0], sizeof(ELEM_T) * SIZE);
            else
                serialize_array(ser, &(*aPtr)[0], SIZE, serialize_array_element<ELEM_T>);
            break;
        }
    }
};

} // namespace pvt

// Serialize fixed arrays and pointers to them
template <typename ELEM_T, size_t SIZE>
struct serialize_impl<ELEM_T[SIZE]> : pvt::serialize_impl_fixed_array<ELEM_T[SIZE], ELEM_T, SIZE>
{};

template <typename ELEM_T, size_t SIZE>
struct serialize_impl<std::array<ELEM_T, SIZE>> :
    pvt::serialize_impl_fixed_array<std::array<ELEM_T, SIZE>, ELEM_T, SIZE>
{};

template <typename ELEM_T, size_t SIZE>
struct serialize_impl<ELEM_T (*)[SIZE]> : pvt::serialize_impl_fixed_array<ELEM_T (*)[SIZE], ELEM_T, SIZE>
{};

template <typename ELEM_T, size_t SIZE>
struct serialize_impl<std::array<ELEM_T, SIZE>*> :
    pvt::serialize_impl_fixed_array<std::array<ELEM_T, SIZE>*, ELEM_T, SIZE>
{};

// Serialize dynamic arrays
template <typename ELEM_T, typename SIZE_T>
struct serialize_impl<pvt::array_wrapper<ELEM_T, SIZE_T>>
{
    void operator()(pvt::array_wrapper<ELEM_T, SIZE_T>& ary, serializer& ser)
    {
        switch ( const auto mode = ser.mode() ) {
        case serializer::MAP:
            // TODO: Implement mapping mode
            // Functions like pvt::serialize_array_map() and pvt::new_fixed_array_map() should be used to reduce code
            // size
            break;

        default:
            if constexpr ( std::is_void_v<ELEM_T> || pvt::is_trivial_element_v<ELEM_T> )
                ser.binary(ary.ptr, ary.size);
            else {
                ser.primitive(ary.size);
                if ( mode == serializer::UNPACK ) ary.ptr = new ELEM_T[ary.size];
                pvt::serialize_array(ser, ary.ptr, ary.size, pvt::serialize_array_element<ELEM_T>);
            }
            break;
        }
    }
};

/**
   Version of serialize that works for copying raw pointers (only
   copying the value of the pointer.  Note that this is only useful
   if the value is going to be sent back to the originator, since it
   won't be valid on the other rank.
 */
template <typename ELEM_T>
struct serialize_impl<pvt::raw_ptr_wrapper<ELEM_T>>
{
    void operator()(pvt::raw_ptr_wrapper<ELEM_T>& a, serializer& ser)
    {
        switch ( ser.mode() ) {
        case serializer::MAP:
            // TODO: Implement mapping mode
            // Functions like pvt::serialize_array_map() and pvt::new_fixed_array_map() should be used to reduce code
            // size
            break;

        default:
            ser.primitive(a.ptr);
            break;
        }
    }
};

// Wrapper functions

template <typename ELEM_T, typename SIZE_T>
pvt::array_wrapper<ELEM_T, SIZE_T>
array(ELEM_T*& ptr, SIZE_T& size)
{
    return { ptr, size };
}

template <typename SIZE_T>
pvt::array_wrapper<void, SIZE_T>
buffer(void*& ptr, SIZE_T& size)
{
    return { ptr, size };
}

template <typename ELEM_T>
pvt::raw_ptr_wrapper<ELEM_T>
raw_ptr(ELEM_T*& ptr)
{
    return { ptr };
}

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ARRAY_H
