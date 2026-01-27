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

#ifndef SST_CORE_SST_UTLI_BIT_UTIL_H
#define SST_CORE_SST_UTLI_BIT_UTIL_H

#include <cstdint>
#include <limits>
#include <type_traits>

namespace SST::bit_util {


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Templates to get the maximum value of a type                                                    //
/////////////////////////////////////////////////////////////////////////////////////////////////////

/**
   Gets the maximum value of a type.  This version can take the variable of the type to infer the needed type.

   @param var Unused in function.  Only used to infer the correct type

   @return Maximum value of the templated type
*/
// template <typename T>
// constexpr T type_max_v(T& var [[maybe_unused]])
// {
//     static_assert(std::is_integral<T>::value, "Template parameter must be an integral type.");
//     return std::numeric_limits<T>::max();
// }

template <typename T>
constexpr T type_max = std::numeric_limits<T>::max();

// Mask with the specified number of top bits set to all ones
template <typename T, unsigned N = sizeof(T) * 8 / 2>
inline constexpr T upper_mask = ~static_cast<T>(0) << ((sizeof(T) * 8) - N);

// Mask with the specified number of bottom bits set to all ones
template <typename T, unsigned N = sizeof(T) * 8 / 2>
inline constexpr T lower_mask =
    static_cast<T>(~static_cast<typename std::make_unsigned<T>::type>(0) >> ((sizeof(T) * 8) - N));


} // namespace SST::bit_util

#endif // SST_CORE_SST_TYPES_H
