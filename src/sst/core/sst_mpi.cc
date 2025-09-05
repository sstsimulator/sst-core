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

#include "sst_config.h"

#include "sst/core/sst_mpi.h"

#include <cstring>

int
SST_MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op UNUSED_WO_MPI(op),
    MPI_Comm UNUSED_WO_MPI(comm))
{
#ifdef SST_CONFIG_HAVE_MPI
    return MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
#else
    // For no MPI, the datatypes are #defined to be the size of the type
    std::memcpy(recvbuf, sendbuf, datatype * count);
    return 0;
#endif
}

int
SST_MPI_Barrier(MPI_Comm UNUSED_WO_MPI(comm))
{
#ifdef SST_CONFIG_HAVE_MPI
    return MPI_Barrier(comm);
#else
    return 0;
#endif
}


int
SST_MPI_Allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
    int UNUSED_WO_MPI(recvcount), MPI_Datatype UNUSED_WO_MPI(recvtype), MPI_Comm UNUSED_WO_MPI(comm))
{
#ifdef SST_CONFIG_HAVE_MPI
    return MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
#else
    std::memcpy(recvbuf, sendbuf, sendtype * sendcount);
    return 0;
#endif
}
