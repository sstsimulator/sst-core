/* FILE: softfloat_macros.h
 * AUTHOR: William McLendon
 *
 *
 */
#ifndef __SOFTFLOAT_MACROS_H__
#define __SOFTFLOAT_MACROS_H__

#include "softfloat.h"

/*--- conversion routines --- */
/*- These are provided for clarity   */
#define DOUBLE_TO_FLOAT64(OUT,IN)     \
{                                     \
    memcpy(&OUT, &IN, sizeof(IN));   \
}

#define FLOAT64_TO_DOUBLE(OUT,IN)     \
{                                     \
    memcpy(&OUT, &IN, sizeof(IN));   \
}

/* Perform an ADD - parameters assumed to be doubles */
#define FLOAT64_FADD(RESULT, A, B)    \
{                                     \
}


#endif /* __SOFTFLOAT_MACROS_H__ */

/* EOF */
