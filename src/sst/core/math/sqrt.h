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


#ifndef _H_SST_CORE_MATH_SQRT
#define _H_SST_CORE_MATH_SQRT

namespace SST {
namespace Math {

// Implements uint32_t square root based on algorithm from:
// Reference: http://en.wikipedia.org/wiki/Methods_of_computing_square_roots
static inline uint32_t square_root(const uint32_t input) {

    uint32_t op  = input;
    uint32_t res = 0;
    uint32_t one = 1uL << 30;

    while (one > op)
    {
        one >>= 2;
    }

    while (one != 0)
    {
        if (op >= res + one)
        {
            op = op - (res + one);
            res = res +  2 * one;
        }

        res >>= 1;
        one >>= 2;
    }

    return res;
};

}
}

#endif
