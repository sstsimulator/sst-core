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

#include <complex>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace SST::Core::Serialization {

/////////////////////////////////////////////////////////////////////////
// Whether two names are the same template. Similar to std::is_same_v. //
/////////////////////////////////////////////////////////////////////////

template <template <typename...> class, template <typename...> class>
constexpr bool is_same_template_v = false;

template <template <typename...> class T>
constexpr bool is_same_template_v<T, T> = true;

//////////////////////////////////////////////////////////////////////////////////////////
// Whether a certain type is the same as a certain class template filled with arguments //
//////////////////////////////////////////////////////////////////////////////////////////
template <class, template <typename...> class>
constexpr bool is_same_type_template_v = false;

template <template <typename...> class T1, typename... T1ARGS, template <typename...> class T2>
constexpr bool is_same_type_template_v<T1<T1ARGS...>, T2> = is_same_template_v<T1, T2>;

//////////////////////////////////////////////////
// Compute the number of fields in an aggregate //
//////////////////////////////////////////////////
namespace pvt_nfields {

// glob is convertible to any other type while initializing an aggregate
// The conversion function is only used in decltype() so it does not need to be defined
struct glob
{
    template <class T>
    operator T() const;
};

// Whether N fields can be used to initialize an aggregate, i.e., number of aggregate fields >= N
template <class C, class, bool = std::is_aggregate_v<C>, class = C>
constexpr bool nfields_ge_impl = false;

// Try to initialize N fields in an aggregate, with glob() automatically converting to each field's
// type. If initialization fails, it is not an error (SFINAE) and this specialization won't apply.
template <class C, size_t... I>
constexpr bool nfields_ge_impl<C, std::index_sequence<I...>, true, decltype(C { (I, glob())... })> = true;

// Whether the number of fields in an aggregate is greater than or equal to N
template <class C, size_t N>
constexpr bool nfields_ge = nfields_ge_impl<C, std::make_index_sequence<N>>;

// Binary search in [L,H) for number of fields in aggregate. Stops at L when M == L.
template <class C, size_t L, size_t H, size_t M = L + (H - L) / 2, bool = M != L>
constexpr size_t b_search = L;

// Next search is in [L,M)
template <class C, size_t L, size_t H, size_t M, bool>
constexpr size_t b_search_next = b_search<C, L, M>;

// Next search is in [M,H)
template <class C, size_t L, size_t H, size_t M>
constexpr size_t b_search_next<C, L, H, M, true> = b_search<C, M, H>;

// Choose [L,M) or [M,H) depending on whether nfields >= M
template <class C, size_t L, size_t H, size_t M>
constexpr size_t b_search<C, L, H, M, true> = b_search_next<C, L, H, M, nfields_ge<C, M>>;

// Find an upper bound on the number of fields, doubling N as long as number of fields >= N
template <class C, size_t N = 1, bool = true>
constexpr size_t nfields = nfields<C, N * 2, nfields_ge<C, N * 2>>;

// When the number of fields < N, perform binary search in [0,N)
template <class C, size_t N>
constexpr size_t nfields<C, N, false> = b_search<C, 0, N>;

} // namespace pvt_nfields

template <class C>
constexpr size_t nfields = pvt_nfields::nfields<C>;

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Template metaprogramming to determine if a type is trivially serializable - that it can be read //
// and written as raw data without any special handling.                                           //
//                                                                                                 //
// The criteria are:                                                                               //
//                                                                                                 //
// It is trivially copyable ( std::is_trivially_copyable )                                         //
//                                                                                                 //
// It is a standard layout type ( std::is_standard_layout )                                        //
//                                                                                                 //
// It is one of these types:                                                                       //
// - arithmetic (integral, floating-point)                                                         //
// - enumeration (including std::byte)                                                             //
// - pointer-to-member (not pointer)                                                               //
// - std::complex<T>, C99 _Complex                                                                 //
// - aggregate with trivially serializable members                                                 //
//                                                                                                 //
// An aggregate is a C-style array, std::array, or a class/struct/union with all public non-static //
// data members and direct bases, no user-provided, inherited or explicit constructors (C++17), no //
// user-declared or inherited constructors (C++20), and no virtual functions or virtual bases.     //
//                                                                                                 //
// Pointers are not considered trivially serializable since they have specific addresses which are //
// not portable across runs, and may require special tracking and allocation. But pointers to      //
// members are typed offsets within a class, and are portable across runs.                         //
//                                                                                                 //
// Note: If an aggregate is a union, only the first member will be considered active, and thus     //
// cannot be a pointer. Rather than ban unions entirely, pointers in unions are strongly           //
// discouraged and can cause unexpected results.                                                   //
/////////////////////////////////////////////////////////////////////////////////////////////////////

namespace pvt_trivial {

// A type must be trivially copyable and standard layout to be trivially serializable
template <class C, bool = std::is_trivially_copyable_v<C> && std::is_standard_layout_v<C>>
constexpr bool is_trivially_serializable_v = false;

// glob_ser is used to generate a conversion to any trivially serializable type while initializing an aggregate
struct glob_ser
{
    // The type of the conversion must be trivially serializable or the conversion fails
    template <class C, class = std::enable_if_t<is_trivially_serializable_v<C>>>
    operator C() const;
};

// Whether all fields of an aggregate type are trivially serializable
template <class C, class, class = C>
constexpr bool trivially_serializable_fields = false;

// Try to initialize N fields in an aggregate, with glob_ser() automatically converting to each field's
// type. If conversion fails, it is not an error (SFINAE) and this specialization won't apply.
template <class C, size_t... I>
constexpr bool trivially_serializable_fields<C, std::index_sequence<I...>, decltype(C { (I, glob_ser())... })> = true;

// has_no_ptr: Whether a type is an aggregate and contains no pointers, or is a non-pointer scalar

// If it's not an aggregate, it needs to be integer, floating-point, enum or member pointer (not regular pointer)
template <class C, bool = std::is_aggregate_v<C>>
constexpr bool has_no_ptr = std::is_arithmetic_v<C> || std::is_enum_v<C> || std::is_member_pointer_v<C>;

// If it's an aggregate, all of its fields must be trivially serializable
template <class C>
constexpr bool has_no_ptr<C, true> = trivially_serializable_fields<C, std::make_index_sequence<nfields<C>>>;

// If a class is trivially copyable and standard layout, it must not have any pointers
template <class C>
constexpr bool is_trivially_serializable_v<C, true> = has_no_ptr<C>;

// Complex numbers are trivially serializable
template <class C>
constexpr bool is_trivially_serializable_v<std::complex<C>, true> = true;

#ifndef __STDC_NO_COMPLEX__
template <>
inline constexpr bool is_trivially_serializable_v<float _Complex, true> = true;

template <>
inline constexpr bool is_trivially_serializable_v<double _Complex, true> = true;

template <>
inline constexpr bool is_trivially_serializable_v<long double _Complex, true> = true;
#endif

// Other floating-point types not covered by std::is_floating_point
template <>
inline constexpr bool is_trivially_serializable_v<_Float16, true> = true;

template <>
inline constexpr bool is_trivially_serializable_v<__float128, true> = true;

} // namespace pvt_trivial

// Whether a type is trivially serializable
template <class C>
constexpr bool is_trivially_serializable_v = pvt_trivial::is_trivially_serializable_v<C>;

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UTILITY_H
