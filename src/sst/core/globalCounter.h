// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_GLOBAL_COUNTER_H
#define SST_GLOBAL_COUNTER_H

#include <limits>
#include <sys/types.h>
#include <type_traits>

namespace SST {

template <typename T = u_int64_t>
class GlobalCounter
{
    T       counter;
    const T min;
    const T max;

public:
    static_assert(std::is_integral<T>::value, "integral required");

    GlobalCounter(T initial = 0) : min(std::numeric_limits<T>::lowest()), max(std::numeric_limits<T>::max())
    {
        counter = initial;
    }

    void increment()
    {
        if ( counter == max ) { return; }
        ++counter;
    }

    void decrement()
    {
        if ( counter == min ) { return; }
        --counter;
    }

    friend bool operator<(const GlobalCounter<T>& l, const GlobalCounter<T>& r) { return l.counter < r.counter; }
};

} // namespace SST

#endif /* SST_GLOBAL_COUNTER_H */
