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

#ifndef SST_CORE_SERIALIZATION_IMPL_GET_ARRAY_SIZE_H
#define SST_CORE_SERIALIZATION_IMPL_GET_ARRAY_SIZE_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/get_array_size.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace SST::Core::Serialization {

// Convert array size to size_t, flagging errors
template <typename SIZE_T>
std::enable_if_t<std::is_integral_v<SIZE_T>, size_t>
get_array_size(SIZE_T size, const char* msg)
{
    bool range_error;
    if constexpr ( std::is_unsigned_v<SIZE_T> )
        range_error = size > SIZE_MAX;
    else if constexpr ( sizeof(SIZE_T) > sizeof(size_t) )
        range_error = size < 0 || size > static_cast<SIZE_T>(SIZE_MAX);
    else
        range_error = size < 0;
    if ( range_error ) throw std::range_error(msg);
    return static_cast<size_t>(size);
}

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_GET_ARRAY_SIZE_H
