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

#endif
