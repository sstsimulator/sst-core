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

#include "sst_config.h"

#include "sst/core/serialization/serialize.h"

#include <array>
#include <bitset>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace SST::Core::Serialization::unittest {

// Compile-time unit tests of trivially serializable types

template <typename TYPE, bool EXPECTED>
void
trivially_serializable_test()
{
    static_assert(is_trivially_serializable_v<TYPE> == EXPECTED);
}

template <typename TYPE>
void
test_trivially_serializable_type_assumptions()
{
    static_assert(std::is_trivially_copyable_v<TYPE> && std::is_standard_layout_v<TYPE>);
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

struct test_complex_mix
{
    float _Complex cf;
    double _Complex cd;
    long double _Complex cl;
    std::complex<double> cfa[10];
};

template void trivially_serializable_test<test_complex_mix, true>();

struct test_complex_double_array
{
    test_complex_double ary[1000];
};

template void trivially_serializable_test<test_complex_double_array, true>();

struct test_struct
{
    test_complex_float z[8];
    std::bitset<100>   bs;
    test_complex_float test_struct::* mem_ptr;
    std::array<size_t, 100>           sizes;
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

template void trivially_serializable_test<std::vector<int>, false>();

struct test_struct_ref
{
    test_complex_float z[8];
    double&            dref;
};

template void trivially_serializable_test<test_struct_ref, false>();

struct test_struct_ptr
{
    int    i;
    int*   iptr;
    double d;
};

template void trivially_serializable_test<test_struct_ptr, false>();

struct test_serialize_order
{
    std::complex<double> cfa[10];
    void                 serialize_order(serializer& ser) { SST_SER(cfa); }
};

template void trivially_serializable_test<test_serialize_order, false>();

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

// Test assumptions which are made by is_trivially_serializable_v<TYPE>
template void test_trivially_serializable_type_assumptions<std::bitset<1>>();
template void test_trivially_serializable_type_assumptions<std::bitset<8>>();
template void test_trivially_serializable_type_assumptions<std::bitset<100>>();
template void test_trivially_serializable_type_assumptions<std::bitset<200>>();
template void test_trivially_serializable_type_assumptions<std::bitset<400>>();
template void test_trivially_serializable_type_assumptions<std::bitset<800>>();
template void test_trivially_serializable_type_assumptions<std::bitset<1600>>();

template void test_trivially_serializable_type_assumptions<std::complex<float>>();
template void test_trivially_serializable_type_assumptions<std::complex<double>>();
template void test_trivially_serializable_type_assumptions<std::complex<long double>>();

} // namespace SST::Core::Serialization::unittest
