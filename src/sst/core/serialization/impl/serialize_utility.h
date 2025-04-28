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

// Whether two names are the same template. Similar to std::is_same_v.
template <template <typename...> class, template <typename...> class>
constexpr bool is_same_template_v = false;

template <template <typename...> class T>
constexpr bool is_same_template_v<T, T> = true;

// Whether a certain type is the same as a certain class template filled with arguments
template <class, template <typename...> class>
constexpr bool is_same_type_template_v = false;

template <template <typename...> class T1, typename... T1ARGS, template <typename...> class T2>
constexpr bool is_same_type_template_v<T1<T1ARGS...>, T2> = is_same_template_v<T1, T2>;

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_UTILITY_H
