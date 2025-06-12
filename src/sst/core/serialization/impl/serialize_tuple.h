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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TUPLE_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TUPLE_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_tuple.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/impl/serialize_utility.h"
#include "sst/core/serialization/serializer.h"

#include <tuple>
#include <type_traits>

namespace SST::Core::Serialization {

// Serialize tuples and pairs
template <typename T>
class serialize_impl<T,
    std::enable_if_t<is_same_type_template_v<T, std::tuple> || is_same_type_template_v<T, std::pair>>>
{
    void operator()(T& t, serializer& ser, ser_opt_t options)
    {
        // Serialize each element of tuple or pair
        ser_opt_t opt = SerOption::is_set(options, SerOption::as_ptr_elem) ? SerOption::as_ptr : SerOption::none;
        std::apply([&](auto&... e) { ((sst_ser_object(ser, e, opt)), ...); }, t);
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_TUPLE_H
