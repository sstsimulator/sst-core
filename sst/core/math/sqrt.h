
#ifndef _H_SST_CORE_MATH_SQRT
#define _H_SST_CORE_MATH_SQRT

#include <sst_config.h>

namespace SST {
namespace Math {

// Implements uint32_t square root based on algorithm from:
// Reference: http://en.wikipedia.org/wiki/Methods_of_computing_square_roots
static uint32_t square_root(const uint32_t input) {

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
