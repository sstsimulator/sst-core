#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/param.h>

#include "softFloat/milieu.h"
#include "softFloat/softfloat.h"


void print_sizes(void)
{
    printf("sizeof float  = %d\n", sizeof(float));
    printf("sizeof double = %d\n", sizeof(double));
    printf("sizeof int    = %d\n", sizeof(int));
    printf("sizeof long   = %d\n", sizeof(long));
    printf("sizeof llint  = %d\n", sizeof(long long));
}


void float_to_float32(float32 *out, float* in)
{
    assert(in != NULL);
    assert(out != NULL);
    memcpy( out, in, sizeof(*out) );
}

void float32_to_float(float *out, float32* in)
{
    assert(in != NULL);
    assert(out != NULL);
    memcpy(out, in, sizeof(*out) );
}

void double_to_float64(float64 *out, double* in)
{
    assert(in != NULL);
    assert(out != NULL);
    memcpy( out, in, sizeof(*out) );
}

void float64_to_double(double *out, float64* in)
{
    assert(in != NULL);
    assert(out != NULL);
    memcpy(out, in, sizeof(*out) );
}

int f32_add(float *u, float a, float b)
{
    int exception;
    float __u;
    float32 _a, _b, _u;
    float_to_float32(&_a, &a);	    // a => _a
    float_to_float32(&_b, &b);	    // b => _b
    _u = float32_add(_a, _b);
    float32_to_float(u, &_u);	    // _u => u
    exception = float_exception_flags;
    float_exception_flags = 0;
    return(exception);
}


int f64_add(double *u, double a, double b)
{
  int exception;
  double __u;
  float64 _a, _b, _u;
  double_to_float64(&_a, &a);
  double_to_float64(&_b, &b);
  _u = float64_add(_a, _b);
  float64_to_double(u, &_u);
  exception = float_exception_flags;
  float_exception_flags = 0;
  return(exception);
}


int f32_sub(float *u, float a, float b)
{
    float32 _a, _b, _u;
    float_to_float32(&_a, &a);	    // a => _a
    float_to_float32(&_b, &b);	    // b => _b
    _u = float32_sub(_a, _b);
    float32_to_float(u, &_u);	    // _u => u
    return(float_exception_flags);
}


int f32_mul(float *u, float a, float b)
{
    float32 _a, _b, _u;
    float_to_float32(&_a, &a);	    // a => _a
    float_to_float32(&_b, &b);	    // b => _b
    _u = float32_mul(_a, _b);
    float32_to_float(u, &_u);	    // _u => u
    return(float_exception_flags);
}


int f32_div(float *u, float a, float b)
{
    float32 _a, _b, _u;
    float_to_float32(&_a, &a);	    // a => _a
    float_to_float32(&_b, &b);	    // b => _b
    _u = float32_div(_a, _b);
    float32_to_float(u, &_u);	    // _u => u
    return(float_exception_flags);
}


int f32_rem(float *u, float a, float b)
{
    float32 _a, _b, _u;
    float_to_float32(&_a, &a);	    // a => _a
    float_to_float32(&_b, &b);	    // b => _b
    _u = float32_rem(_a, _b);
    float32_to_float(u, &_u);	    // _u => u
    return(float_exception_flags);
}

void print_exception(int exception)
{
    int c = 0;
    c += (exception &  1) != 0;
    c += (exception &  2) != 0;
    c += (exception &  4) != 0;
    c += (exception &  8) != 0;
    c += (exception & 16) != 0;

    if( (exception & 1) != 0)
    {
	printf("INEXACT");
	if(--c > 0) 
	    printf(",");
    }
    if( (exception & 2) != 0)
    {
	printf("DIVZERO");
	if(--c > 0) 
	    printf(",");
    }
    if( (exception & 4) != 0)
    {
	printf("UNDRFLO");
	if(--c > 0) 
	    printf(",");
    }
    if( (exception & 8) != 0)
    {
	printf("OVERFLO");
	if(--c > 0) 
	    printf(",");
    }
    if( (exception & 16) != 0)
	printf("INVALID");
}


int main(int argc, char* argv[])
{
    int   exception = 0;
    float val;
    double dval;

    double a, b;

    assert(sizeof(float)  == sizeof(float32));
    assert(sizeof(double) == sizeof(float64));
    //print_sizes();

    a = 0.4;
    b = 1.6;
    exception = f64_add(&dval, a,b);
    printf("val = %lf\n", dval);

    exception = f64_add(&dval, 0.4, 99.6);
    printf("val = %lf\n", dval);
    
#if 0
    // Test the ADDER
    printf("--- add --------------------------------------\n");
    exception = f32_add(&val, 1E6, 1.0);
    printf("val = %14f\t%#3x:[", val,exception); print_exception(exception); printf("]\n");

    exception = f32_add(&val, 1E38, -1E38);
    printf("val = %14f\t%#3x:[", val,exception); print_exception(exception); printf("]\n");

    exception = f32_add(&val, 0.0, -1.0);
    printf("val = %14f\t%#3x:[", val,exception); print_exception(exception); printf("]\n");

    exception = f32_add(&val, 1.8E38, 1.8E38);
    printf("val = %14f\t%#3x:[", val,exception); print_exception(exception); printf("]\n");

    exception = f64_add(&dval, 1.8E38, 1.8E38);
    printf("dval = %14f\t%#3x:[", dval, exception); print_exception(exception); printf("]\n");

    printf("--- div --------------------------------------\n");
    exception = f32_div(&val, 7.00, 10.00);
    printf("val = %14f\t%#3x:[",val,exception); print_exception(exception); printf("]\n");
    float_exception_flags = 0;

    exception = f32_div(&val, 10.00, 1);
    printf("val = %14f\t%#3x:[",val,exception); print_exception(exception); printf("]\n");
    float_exception_flags = 0;

    exception = f32_div(&val, 1E-20, 1E-20);
    printf("val = %14f\t%#3x:[",val,exception); print_exception(exception); printf("]\n");
    float_exception_flags = 0;

    exception = f32_div(&val, 1,10000);
    printf("val = %14f\t%#3x:[",val,exception); print_exception(exception); printf("]\n");
    float_exception_flags = 0;

    exception = f32_div(&val, 100,0);
    printf("val = %14f\t%#3x:[",val,exception); print_exception(exception); printf("]\n");
    float_exception_flags = 0;
#endif

    return(0);
}
// EOF
