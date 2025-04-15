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

#include <type_traits>

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

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UTILITY_H
