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

#ifndef SST_CORE_SST_MPI_H
#define SST_CORE_SST_MPI_H

#include "warnmacros.h"

#ifdef SST_CONFIG_HAVE_MPI

DISABLE_WARN_MISSING_OVERRIDE
DISABLE_WARN_CAST_FUNCTION_TYPE

#include <mpi.h>

REENABLE_WARNING
REENABLE_WARNING

#define UNUSED_WO_MPI(x) x

#else

#define UNUSED_WO_MPI(x) UNUSED(x)

#endif

/*************************************************************************
   Make it so that we can do simple MPI functions without having to
   guard using SST_CONFIG_HAVE_MPI.  This will include SST_MPI_*
   versions of the supported operations, as well as definitions for
   some of the MPI types
 *************************************************************************/

struct mpi_short_int_t
{
    short val;
    int   rank;
};

struct mpi_long_int_t
{
    long val;
    int  rank;
};

struct mpi_float_int_t
{
    double val;
    int    rank;
};

struct mpi_double_int_t
{
    double val;
    int    rank;
};

#ifndef SST_CONFIG_HAVE_MPI

// Define some MPI types and some of the values for those types
#define MPI_Datatype int
#define MPI_Op       int
#define MPI_Comm     int

#define MPI_COMM_WORLD 0

// MPI_datatype values. These will just be set to the sizeof() the represented type

#define MPI_SIGNED_CHAR    (sizeof(signed char))
#define MPI_UNSIGNED_CHAR  (sizeof(unsigned char))
#define MPI_SHORT          (sizeof(short))
#define MPI_UNSIGNED_SHORT (sizeof(unsigned short))
#define MPI_INT            (sizeof(int))
#define MPI_UNSIGNED       (sizeof(unsigned))
#define MPI_LONG           (sizeof(long))
#define MPI_UNSIGNED_LONG  (sizeof(unsigned long))
// MPI_LONG_LONG_INT (a.k.a MPI_LONG_LONG) / MPI_UNSIGNED_LONG_LONG
#define MPI_CHAR           (sizeof(char))
#define MPI_WCHAR          (sizeof(wchar))
#define MPI_FLOAT          (sizeof(float))
#define MPI_DOUBLE         (sizeof(double))
// MPI_LONG_DOUBLE
#define MPI_INT8_T         (sizeof(int8_t))
#define MPI_UINT8_T        (sizeof(uint8_t))
#define MPI_INT16_T        (sizeof(int16_t))
#define MPI_UINT16_T       (sizeof(uint16_t))
#define MPI_INT32_T        (sizeof(int32_t))
#define MPI_UINT32_T       (sizeof(uint32_t))
#define MPI_INT64_T        (sizeof(int64_t))
#define MPI_UINT64_T       (sizeof(uint64_t))
#define MPI_C_BOOL
// MPI_C_COMPLEX
// MPI_C_FLOAT_COMPLEX
// MPI_C_DOUBLE_COMPLEX
// MPI_C_LONG_DOUBLE_COMPLEX
// MPI_AINT
// MPI_COUNT
// MPI_OFFSET
// MPI_BYTE
// MPI_PACKED

// List of datatypes for reduction functions MPI_MINLOC and MPI_MAXLOC:
#define MPI_SHORT_INT  (sizeof(mpi_short_int_t))
#define MPI_LONG_INT   (sizeof(mpi_long_int_t))
#define MPI_FLOAT_INT  (sizeof(mpi_float_int_t))
#define MPI_DOUBLE_INT (sizeof(mpi_double_int_t))
// MPI_LONG_DOUBLE_INT
// MPI_2INT

// Allreduce operations. Numbers are unused because they are all
// no-ops in the case of MPI
#define MPI_SUM    0
#define MPI_MAX    0
#define MPI_MIN    0
#define MPI_MAXLOC 0
#define MPI_MINLOC 0

#endif

// Verions of typical MPI functions that will hide the
// SST_CONFIG_HAVE_MPI macros

int SST_MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int SST_MPI_Barrier(MPI_Comm comm);
int SST_MPI_Allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm);

#endif
