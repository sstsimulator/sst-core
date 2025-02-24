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

#ifndef SST_CORE_FROM_STRING_H
#define SST_CORE_FROM_STRING_H

#include <stdexcept>
#include <string>
#include <type_traits>

namespace SST {
namespace Core {

template <class T>
std::enable_if_t<std::is_integral_v<T>, T>
from_string(const std::string& input)
{
    if constexpr ( std::is_signed_v<T> ) {
        if constexpr ( std::is_same_v<int, T> ) { return std::stoi(input, nullptr, 0); }
        else if constexpr ( std::is_same_v<long, T> ) {
            return std::stol(input, nullptr, 0);
        }
        else if constexpr ( std::is_same_v<long long, T> ) {
            return std::stoll(input, nullptr, 0);
        }
        else { // Smaller than 32-bit
            return static_cast<T>(std::stol(input, nullptr, 0));
        }
    }
    else {
        if constexpr ( std::is_same_v<bool, T> ) {
            // valid pairs: true/false, t/f, yes/no, y/n, on/off, 1/0
            std::string transform(input);
            for ( auto& c : transform )
                c = std::tolower(c);
            if ( transform == "true" || transform == "t" || transform == "yes" || transform == "y" ||
                 transform == "on" || transform == "1" ) {
                return true;
            }
            else if (
                transform == "false" || transform == "f" || transform == "no" || transform == "n" ||
                transform == "off" || transform == "0" ) {
                return false;
            }
            else {
                throw std::invalid_argument("from_string: no valid conversion");
            }
        }
        else if constexpr ( std::is_same_v<unsigned long, T> ) {
            return std::stoul(input, nullptr, 0);
        }
        else if constexpr ( std::is_same_v<unsigned long long, T> ) {
            return std::stoull(input, nullptr, 0);
        }
        else { // Smaller than 32-bit
            return static_cast<T>(std::stoul(input, nullptr, 0));
        }
    }
}

template <class T>
std::enable_if_t<std::is_floating_point_v<T>, T>
from_string(const std::string& input)
{
    if constexpr ( std::is_same_v<float, T> ) { return stof(input); }
    else if constexpr ( std::is_same_v<double, T> ) {
        return stod(input);
    }
    else if constexpr ( std::is_same_v<long double, T> ) {
        return stold(input);
    }
    else { // make compiler happy
        return stod(input);
    }
}

template <class T>
std::enable_if_t<std::is_class_v<T>, T>
from_string(const std::string& input)
{
    static_assert(
        std::is_constructible_v<T, std::string>,
        "ERROR: from_string can only be used with integral and floating point types and with classes that have a "
        "constructor that takes a single string as input.\n");
    return T(input);
}

} // end namespace Core
} // end namespace SST

#endif // SST_CORE_FROM_STRING_H
