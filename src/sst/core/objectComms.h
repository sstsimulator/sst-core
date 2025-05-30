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

#ifndef SST_CORE_OBJECTCOMMS_H
#define SST_CORE_OBJECTCOMMS_H

#include "sst/core/objectSerialization.h"
#include "sst/core/sst_mpi.h"

#include <memory>
#include <typeinfo>

namespace SST::Comms {

#ifdef SST_CONFIG_HAVE_MPI
template <typename dataType>
void
broadcast(dataType& data, int root)
{
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if ( root == rank ) {
        // Serialize the data
        std::vector<char> buffer = Comms::serialize(data);

        // Now broadcast the size of the data
        int size = buffer.size();
        MPI_Bcast(&size, 1, MPI_INT, root, MPI_COMM_WORLD);

        // Now broadcast the data
        MPI_Bcast(buffer.data(), buffer.size(), MPI_BYTE, root, MPI_COMM_WORLD);
    }
    else {
        // Get the size of the broadcast
        int size = 0;
        MPI_Bcast(&size, 1, MPI_INT, root, MPI_COMM_WORLD);

        // Now get the data
        auto buffer = std::unique_ptr<char[]>(new char[size]);
        MPI_Bcast(buffer.get(), size, MPI_BYTE, root, MPI_COMM_WORLD);

        // Now deserialize data
        Comms::deserialize(buffer.get(), size, data);
    }
}

template <typename dataType>
void
send(int dest, int tag, dataType& data)
{
    // Serialize the data
    std::vector<char> buffer = Comms::serialize<dataType>(data);

    // Now send the data.  Send size first, then payload
    // std::cout<< sizeof(buffer.size()) << std::endl;
    int64_t size = buffer.size();
    MPI_Send(&size, 1, MPI_INT64_T, dest, tag, MPI_COMM_WORLD);

    int32_t fragment_size = 1000000000;
    int64_t offset        = 0;

    while ( size >= fragment_size ) {
        MPI_Send(buffer.data() + offset, fragment_size, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
        size -= fragment_size;
        offset += fragment_size;
    }
    MPI_Send(buffer.data() + offset, size, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
}

template <typename dataType>
void
recv(int src, int tag, dataType& data)
{
    // Get the size of the broadcast
    int64_t    size = 0;
    MPI_Status status;
    MPI_Recv(&size, 1, MPI_INT64_T, src, tag, MPI_COMM_WORLD, &status);

    // Now get the data
    auto    buffer        = std::unique_ptr<char[]>(new char[size]);
    int64_t offset        = 0;
    int32_t fragment_size = 1000000000;
    int64_t rem_size      = size;

    while ( rem_size >= fragment_size ) {
        MPI_Recv(buffer.get() + offset, fragment_size, MPI_BYTE, src, tag, MPI_COMM_WORLD, &status);
        rem_size -= fragment_size;
        offset += fragment_size;
    }
    MPI_Recv(buffer.get() + offset, rem_size, MPI_BYTE, src, tag, MPI_COMM_WORLD, &status);

    // Now deserialize data
    Comms::deserialize(buffer.get(), size, data);
}

template <typename dataType>
void
all_gather(dataType& data, std::vector<dataType>& out_data)
{
    int rank = 0, world = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world);

    // Serialize the data
    std::vector<char> buffer = Comms::serialize(data);

    size_t sendSize = buffer.size();

    auto allSizes      = std::make_unique<int[]>(world);
    auto displacements = std::make_unique<int[]>(world);

    memset(allSizes.get(), '\0', world * sizeof(int));
    memset(displacements.get(), '\0', world * sizeof(int));

    MPI_Allgather(&sendSize, sizeof(int), MPI_BYTE, allSizes.get(), sizeof(int), MPI_BYTE, MPI_COMM_WORLD);

    int totalBuf = 0;
    for ( int i = 0; i < world; i++ ) {
        totalBuf += allSizes[i];
        if ( i > 0 ) displacements[i] = displacements[i - 1] + allSizes[i - 1];
    }

    auto bigBuff = std::unique_ptr<char[]>(new char[totalBuf]);

    MPI_Allgatherv(buffer.data(), buffer.size(), MPI_BYTE, bigBuff.get(), allSizes.get(), displacements.get(), MPI_BYTE,
        MPI_COMM_WORLD);

    out_data.resize(world);
    for ( int i = 0; i < world; i++ ) {
        auto* bbuf = bigBuff.get();
        Comms::deserialize(&bbuf[displacements[i]], allSizes[i], out_data[i]);
    }
}

#endif

} // namespace SST::Comms

#endif // SST_CORE_OBJECTCOMMS_H
