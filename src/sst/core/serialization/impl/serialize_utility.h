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

#include "sst/core/sst_complex.h"

#include <bitset>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace SST::Core::Serialization {

/////////////////////////////////////////////////////////////////////////
// Whether two names are the same template. Similar to std::is_same_v. //
/////////////////////////////////////////////////////////////////////////
template <template <class...> class, template <class...> class>
constexpr bool is_same_template_v = false;

template <template <class...> class T>
constexpr bool is_same_template_v<T, T> = true;

template <template <class...> class T1, template <class...> class T2>
using is_same_template = std::bool_constant<is_same_template_v<T1, T2>>;

///////////////////////////////////////////////////////////////////////////////////////
// Whether a type is the same as a certain class template filled with type arguments //
///////////////////////////////////////////////////////////////////////////////////////
template <class, template <class...> class>
constexpr bool is_same_type_template_v = false;

template <template <class...> class T1, class... T1ARGS, template <class...> class T2>
constexpr bool is_same_type_template_v<T1<T1ARGS...>, T2> = is_same_template_v<T1, T2>;

template <class T, template <class...> class TT>
using is_same_type_template = std::bool_constant<is_same_type_template_v<T, TT>>;

///////////////////////////////////////////////////////
// Pre-C++20 is_unbounded_array trait implementation //
///////////////////////////////////////////////////////
#if __cplusplus < 202002l

template <class T>
constexpr bool is_unbounded_array_v = false;

template <class T>
constexpr bool is_unbounded_array_v<T[]> = true;

template <class T>
using is_unbounded_array = std::bool_constant<is_unbounded_array_v<T>>;

#else

using std::is_unbounded_array;
using std::is_unbounded_array_v;

#endif

///////////////////////////////////////////////////
// Whether a type has a serialize_order() method //
///////////////////////////////////////////////////

template <class, class = void>
struct has_serialize_order_impl : std::false_type
{};

template <class T>
struct has_serialize_order_impl<T, decltype(std::declval<T>().serialize_order(std::declval<class serializer&>()))> :
    std::true_type
{};

// Workaround for GCC < 12 bug which does not handle access violations as SFINAE substitution failures
// If serializable_base is a base class of T, we assume that T has a public serialize_order() method.
// If serialize_order() is private or protected in a T derived from serializable_base, it will cause a
// compile-time error in serialize_impl when invoking T{}.serialize_order() even though it's true here.
template <class T>
using has_serialize_order = std::disjunction<std::is_base_of<class serializable_base, T>, has_serialize_order_impl<T>>;

template <class T>
constexpr bool has_serialize_order_v = has_serialize_order<T>::value;

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
// If the type is not an aggregate, always returns false, so that nfields<C> returns 0.
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

// The number of fields in an aggregate, initializable by { ... }. Returns 0 if the type is not aggregate.
template <class C>
constexpr size_t nfields = pvt_nfields::nfields<C>;

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Template metaprogramming to determine if a type is trivially serializable - that it can be read //
// and written as raw data without any special handling.                                           //
//                                                                                                 //
// It is one of these types:                                                                       //
// - arithmetic (integral, floating-point)                                                         //
// - enumeration (including std::byte)                                                             //
// - member object pointer                                                                         //
// - std::complex<T>, C99 _Complex                                                                 //
// - std::bitset                                                                                   //
// - trivially copyable, standard layout aggregate with trivially serializable members and no      //
//   serialize_order() method                                                                      //
//                                                                                                 //
// An aggregate is a C-style array, std::array, or a class/struct/union with all-public non-static //
// data members and direct bases, no user-provided, inherited or explicit constructors (C++17),    //
// no user-declared or inherited constructors (C++20), and no virtual functions or virtual bases.  //
//                                                                                                 //
// Pointers are not considered trivially serializable since they have specific addresses which     //
// are not portable across runs, and may require special tracking and allocation. Pointers to      //
// member functions may have ABI-specific pointers to data which are not portable across runs.     //
// But pointers to member objects are typed offsets within a class, and are portable across runs.  //
//                                                                                                 //
// Note: If an aggregate is a union, only the first member will be considered active, and thus     //
// cannot be a pointer. Rather than ban unions entirely, pointers in unions are strongly           //
// discouraged and can cause unexpected results.                                                   //
//                                                                                                 //
// Note: is_trivially_serializable<T> should return true if T is trivially serializable, even if   //
// there exists a specialization for serialize_impl<T>. is_trivially_serialzable<T> is a trait     //
// for trivial serializability in general, not a serialize_impl specialization test condition.     //
// For example, it returns true for arrays of int, even if serialize_impl<int[N]> is specialized.  //
//                                                                                                 //
// For further reading on the meaning of "trivial":  https://isocpp.org/files/papers/P3279R0.html  //
/////////////////////////////////////////////////////////////////////////////////////////////////////

namespace pvt_trivial {

// If it's not a trivially copyable, standard layout aggregate without a serialize_order() method, it needs to be an
// integer, floating-point, complex, enum or member object pointer, or one of the specializations listed later.
template <class C, bool = std::conjunction_v<std::is_aggregate<C>, std::is_trivially_copyable<C>,
                       std::is_standard_layout<C>, std::negation<has_serialize_order<C>>>>
constexpr bool is_trivially_serializable_v =
    complex_properties<C>::is_complex ||
    std::disjunction_v<std::is_arithmetic<C>, std::is_enum<C>, std::is_member_object_pointer<C>>;

// glob_ts is convertible to any trivially serializable type
struct glob_ts
{
    // The type of the conversion must be trivially serializable or the conversion fails
    template <class C, class = std::enable_if_t<is_trivially_serializable_v<C>>>
    operator C() const;
};

// Whether all fields of an aggregate type are trivially serializable
template <class C, class, class = C>
constexpr bool has_ts_fields = false;

// Try to initialize N fields in an aggregate, with glob_ts() automatically converting to each field's
// type. If conversion fails, it is not an error (SFINAE) and this specialization won't apply.
template <class C, size_t... I>
constexpr bool has_ts_fields<C, std::index_sequence<I...>, decltype(C { (I, glob_ts())... })> = true;

// If it's a trivially copyable, standard layout aggregate, then all of its fields must be trivially serializable
template <class C>
constexpr bool is_trivially_serializable_v<C, true> = has_ts_fields<C, std::make_index_sequence<nfields<C>>>;

// std::bitset is trivially serializable
template <size_t N>
constexpr bool is_trivially_serializable_v<std::bitset<N>, false> = true;

// Other floating-point types not covered by std::is_floating_point
#ifdef FLT16_MIN
template <>
inline constexpr bool is_trivially_serializable_v<_Float16, false> = true;
#endif

// 128-bit integers
#ifdef __SIZEOF_INT128__
template <>
inline constexpr bool is_trivially_serializable_v<__int128, false> = true;

template <>
inline constexpr bool is_trivially_serializable_v<unsigned __int128, false> = true;
#endif

} // namespace pvt_trivial

// Whether a type is trivially serializable
template <class C>
constexpr bool is_trivially_serializable_v = pvt_trivial::is_trivially_serializable_v<C>;

template <class C>
using is_trivially_serializable = std::bool_constant<is_trivially_serializable_v<C>>;

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UTILITY_H
