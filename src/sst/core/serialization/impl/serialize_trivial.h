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

#include "sst/core/serialization/impl/serialize_utility.h"
#include "sst/core/serialization/serializer.h"

#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

namespace SST::Core::Serialization {

// Types excluded because they would cause ambiguity with more specialized methods
template <typename T>
constexpr bool is_trivially_serializable_excluded_v = std::is_array_v<T>;

template <typename T, size_t S>
constexpr bool is_trivially_serializable_excluded_v<std::array<T, S>> = true;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Version of serialize that works for trivially serializable types which aren't excluded, and pointers thereof //
//                                                                                                              //
// Note that the pointer tracking happens at a higher level, and only if it is turned on. If it is not turned   //
// on, then this only copies the value pointed to into the buffer. If multiple objects point to the same        //
// location, they will each have an independent copy after deserialization.                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
class serialize_impl<T, std::enable_if_t<is_trivially_serializable_v<std::remove_pointer_t<T>> &&
                                         !is_trivially_serializable_excluded_v<std::remove_pointer_t<T>>>>
{
    void operator()(T& t, serializer& ser, ser_opt_t options)
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

/////////////////////////////
// Compile-time unit tests //
/////////////////////////////

namespace unittest {

template <typename TYPE, bool EXPECTED>
void
trivially_serializable_test()
{
    static_assert(is_trivially_serializable_v<TYPE> == EXPECTED);
}

template void trivially_serializable_test<std::byte, true>();
template void trivially_serializable_test<int8_t, true>();
template void trivially_serializable_test<uint8_t, true>();
template void trivially_serializable_test<int16_t, true>();
template void trivially_serializable_test<uint16_t, true>();
template void trivially_serializable_test<int32_t, true>();
template void trivially_serializable_test<uint32_t, true>();
template void trivially_serializable_test<int64_t, true>();
template void trivially_serializable_test<uint64_t, true>();
template void trivially_serializable_test<float, true>();
template void trivially_serializable_test<double, true>();
template void trivially_serializable_test<long double, true>();

enum class test_enum { X, Y, Z };
enum class test_enum_int8_t : int8_t { X, Y, Z, A, B, C };
enum class test_enum_uint8_t : uint8_t { X, Y, Z, A, B, C, J, K, L };
enum class test_enum_int16_t : int16_t { X, Y, Z, A, B, C };
enum class test_enum_uint16_t : uint16_t { X, Y, Z, A, B, C, J, K, L };
enum class test_enum_int32_t : int32_t { X, Y, Z, A, B, C };
enum class test_enum_uint32_t : uint32_t { X, Y, Z, A, B, C, J, K, L };
enum class test_enum_int64_t : int64_t { X, Y, Z, A, B, C };
enum class test_enum_uint64_t : uint64_t { X, Y, Z, A, B, C, J, K, L };

template void trivially_serializable_test<test_enum, true>();
template void trivially_serializable_test<test_enum_int8_t, true>();
template void trivially_serializable_test<test_enum_uint8_t, true>();
template void trivially_serializable_test<test_enum_int16_t, true>();
template void trivially_serializable_test<test_enum_uint16_t, true>();
template void trivially_serializable_test<test_enum_int32_t, true>();
template void trivially_serializable_test<test_enum_uint32_t, true>();
template void trivially_serializable_test<test_enum_int64_t, true>();
template void trivially_serializable_test<test_enum_uint64_t, true>();

struct test_complex_float
{
    float r, i;
};
struct test_complex_double
{
    double r, i;
};

template void trivially_serializable_test<test_complex_float, true>();
template void trivially_serializable_test<test_complex_double, true>();
template void trivially_serializable_test<std::complex<float>, true>();
template void trivially_serializable_test<std::complex<double>, true>();

struct test_complex_double_array
{
    test_complex_double ary[1000];
};

template void trivially_serializable_test<test_complex_double_array, true>();

struct test_struct
{
    test_complex_float z[8];
    test_complex_float test_struct::* mem_ptr;
    test_complex_float (test_struct::*mem_fun_ptr)(test_complex_float);
    std::array<size_t, 100> sizes;
    union {
        int   i;
        float f;
    };
};

template void trivially_serializable_test<test_struct, true>();

union test_union_ptr_2nd {
    uintptr_t iptr;
    void*     ptr;
};

template void trivially_serializable_test<test_union_ptr_2nd, true>();

// Tests of types which are not trivially serializable

template void trivially_serializable_test<void, false>();
template void trivially_serializable_test<void*, false>();

typedef void (*fptr)(int, int);
template void trivially_serializable_test<fptr, false>();

typedef test_complex_double* test_complex_double_array_ptr[1000];
template void                trivially_serializable_test<test_complex_double_array_ptr, false>();

struct test_struct_ref
{
    test_complex_float z[8];
    test_complex_float test_struct::* mem_ptr;
    double&                           dref;
    test_complex_float (test_struct::*mem_fun_ptr)(test_complex_float);
};

template void trivially_serializable_test<test_struct_ref, false>();

struct test_struct_ptr
{
    test_complex_float test_struct::* mem_ptr;
    int*                              iptr;
};

template void trivially_serializable_test<test_struct_ptr, false>();

struct test_mix
{
    test_complex_float z;
    test_struct_ref    refs;
};

template void trivially_serializable_test<test_mix, false>();

union test_union_ptr_1st {
    void*     ptr;
    uintptr_t iptr;
};

template void trivially_serializable_test<test_union_ptr_1st, false>();

struct test_container_struct
{
    int                 size;
    std::vector<double> v;
};

template void trivially_serializable_test<test_container_struct, false>();

template void trivially_serializable_test<std::pair<int, int>, false>();

template void trivially_serializable_test<std::tuple<float, float, float>, false>();

} // namespace unittest

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TRIVIAL_H
