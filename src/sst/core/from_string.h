// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_FROM_STRING_H
#define SST_CORE_FROM_STRING_H

#include <string>
#include <type_traits>

namespace SST {
namespace Core {

template<class T>
typename std::enable_if<std::is_integral<T>::value, T >::type
  from_string(const std::string& input)
{
    if ( std::is_signed<T>::value ) {
        if ( std::is_same<int,T>::value ) {
            return std::stoi(input,0,0);
        }
        else if ( std::is_same<long,T>::value ) {
            return std::stol(input,0,0);
        }
        else if ( std::is_same<long long, T>::value ) {
            return std::stoll(input,0,0);
        }
        else {  // Smaller than 32-bit
            return static_cast<T>(std::stol(input,0,0));
        }
    }
    else {
        if ( std::is_same<bool, T>::value ) {
            // valid pairs: true/false, t/f, yes/no, y/n, on/off, 1/0 
            std::string transform(input);
            for (auto & c: transform) c = std::tolower(c);
            if ( transform == "true" || transform == "t" || transform == "yes" || transform == "y" || transform == "on" || transform == "1" ) {
                return true;
            } else if ( transform == "false" || transform == "f" || transform == "no" || transform == "n" || transform == "off" || transform == "0" ) {
                return false;
            }
            else {
                throw new std::invalid_argument("from_string: no valid conversion");
            }
        }
        else if ( std::is_same<unsigned long, T>::value ) {
            return std::stoul(input,0,0);
        }
        else if ( std::is_same<unsigned long long, T>::value ) {
            return std::stoull(input,0,0);
        }
        else {  // Smaller than 32-bit
            return static_cast<T>(std::stoul(input,0,0));
        }
    }
}

template<class T>
typename std::enable_if<std::is_floating_point<T>::value, T >::type
  from_string(const std::string& input)
{  
    if ( std::is_same<float, T>::value ) {
        return stof(input);
    }
    else if ( std::is_same<double, T>::value ) {
        return stod(input);
    }
    else if ( std::is_same<long double, T>::value ) {
        return stold(input);
    }
}

template<class T>
typename std::enable_if<std::is_class<T>::value, T >::type
  from_string(const std::string& input) {
    static_assert(std::is_constructible<T,std::string>::value,"ERROR: from_string can only be used with integral and floating point types and with classes that have a constructor that takes a single string as input.\n");
    return T(input);
}

} // end namespace Core
} // end namespace SST

#endif // SST_CORE_FROM_STRING_H
