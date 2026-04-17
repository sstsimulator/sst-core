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

int
SST_MPI_Bcast(void* UNUSED_WO_MPI(buffer), int UNUSED_WO_MPI(count), MPI_Datatype UNUSED_WO_MPI(datatype),
    int UNUSED_WO_MPI(root), MPI_Comm UNUSED_WO_MPI(comm))
{
#ifdef SST_CONFIG_HAVE_MPI
    return MPI_Bcast(buffer, count, datatype, root, comm);
#else
    // Bcast is a no-op if there is no MPI
    return 0;
#endif
}

int
SST_MPI_Send(const void* UNUSED_WO_MPI(buf), int UNUSED_WO_MPI(count), MPI_Datatype UNUSED_WO_MPI(datatype),
    int UNUSED_WO_MPI(dest), int UNUSED_WO_MPI(tag), MPI_Comm UNUSED_WO_MPI(comm))
{
#ifdef SST_CONFIG_HAVE_MPI
    return MPI_Send(buf, count, datatype, dest, tag, comm);
#else
    return 0;
#endif
}

int
SST_MPI_Recv(void* UNUSED_WO_MPI(buf), int UNUSED_WO_MPI(count), MPI_Datatype UNUSED_WO_MPI(datatype),
    int UNUSED_WO_MPI(source), int UNUSED_WO_MPI(tag), MPI_Comm UNUSED_WO_MPI(comm), MPI_Status* UNUSED_WO_MPI(status))
{
#ifdef SST_CONFIG_HAVE_MPI
    return MPI_Recv(buf, count, datatype, source, tag, comm, status);
#else
    return 0;
#endif
}


int
SST_MPI_GetRank()
{
#ifdef SST_CONFIG_HAVE_MPI
    int myrank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    return myrank;
#else
    return 0;
#endif
}
