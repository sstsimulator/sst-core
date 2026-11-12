// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_OBJECTCOMMS_H
#define SST_CORE_OBJECTCOMMS_H

#include "sst/core/objectSerialization.h"
#include "sst/core/sst_mpi.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <typeinfo>

namespace SST::Comms {

namespace pvt {
// Maximum size of a single MPI message used by broadcast(), send() and recv()
inline size_t max_message_size = 256 * 1024 * 1024;
} // namespace pvt

/**
   Set the maximum size of a single MPI message used by broadcast(), send() and recv(). Larger data is transferred as
   several messages, one per chunk of a chunked_buffer. This is set from the mpi-max-message-size option. The size used
   by the sending rank is sent along with the data, so the ranks do not need to agree on it.
*/
inline void
setMaxMessageSize(size_t size)
{
    pvt::max_message_size = size;
}

inline size_t
getMaxMessageSize()
{
    return pvt::max_message_size;
}

#ifdef SST_CONFIG_HAVE_MPI
template <typename dataType>
void
broadcast(dataType& data, int root)
{
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if ( root == rank ) {
        // Serialize the data
        Core::Serialization::chunked_buffer buffer(getMaxMessageSize());
        Comms::serialize(data, buffer);

        // Broadcast the size of the data and the chunk size, then broadcast each chunk
        uint64_t info[2] = { buffer.size(), buffer.max_chunk_size() };
        MPI_Bcast(info, 2, MPI_UINT64_T, root, MPI_COMM_WORLD);

        for ( size_t i = 0; i < buffer.num_chunks(); ++i )
            MPI_Bcast(buffer.chunk_data(i), static_cast<int>(buffer.chunk_bytes(i)), MPI_BYTE, root, MPI_COMM_WORLD);
    }
    else {
        // Get the size of the data and the chunk size used by the root
        uint64_t info[2] = { 0, 0 };
        MPI_Bcast(info, 2, MPI_UINT64_T, root, MPI_COMM_WORLD);

        // Now get each chunk of the data
        Core::Serialization::chunked_buffer buffer(info[1]);
        buffer.resize(info[0]);

        for ( size_t i = 0; i < buffer.num_chunks(); ++i )
            MPI_Bcast(buffer.chunk_data(i), static_cast<int>(buffer.chunk_bytes(i)), MPI_BYTE, root, MPI_COMM_WORLD);

        // Now deserialize data
        Comms::deserialize(buffer, data);
    }
}

template <typename dataType>
void
send(int dest, int tag, dataType& data)
{
    // Serialize the data
    Core::Serialization::chunked_buffer buffer(getMaxMessageSize());
    Comms::serialize(data, buffer);

    // Send the size of the data and the chunk size first, then send each chunk as a separate message
    uint64_t info[2] = { buffer.size(), buffer.max_chunk_size() };
    MPI_Send(info, 2, MPI_UINT64_T, dest, tag, MPI_COMM_WORLD);

    for ( size_t i = 0; i < buffer.num_chunks(); ++i )
        MPI_Send(buffer.chunk_data(i), static_cast<int>(buffer.chunk_bytes(i)), MPI_BYTE, dest, tag, MPI_COMM_WORLD);
}

template <typename dataType>
void
recv(int src, int tag, dataType& data)
{
    // Get the size of the data and the chunk size used by the sender
    uint64_t   info[2] = { 0, 0 };
    MPI_Status status;
    MPI_Recv(info, 2, MPI_UINT64_T, src, tag, MPI_COMM_WORLD, &status);

    // Now get each chunk of the data. The chunks are received from the rank which sent the size, in case src is
    // MPI_ANY_SOURCE.
    Core::Serialization::chunked_buffer buffer(info[1]);
    buffer.resize(info[0]);

    for ( size_t i = 0; i < buffer.num_chunks(); ++i )
        MPI_Recv(buffer.chunk_data(i), static_cast<int>(buffer.chunk_bytes(i)), MPI_BYTE, status.MPI_SOURCE, tag,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Now deserialize data
    Comms::deserialize(buffer, data);
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

    uint64_t sendSize = buffer.size();

    auto allSizes      = std::make_unique<uint64_t[]>(world);
    auto counts        = std::make_unique<int[]>(world);
    auto displacements = std::make_unique<int[]>(world);

    MPI_Allgather(&sendSize, 1, MPI_UINT64_T, allSizes.get(), 1, MPI_UINT64_T, MPI_COMM_WORLD);

    // MPI_Allgatherv gathers all of the data into a single buffer, so its total size must fit in an int
    uint64_t totalBuf = 0;
    for ( int i = 0; i < world; i++ ) {
        counts[i]        = static_cast<int>(allSizes[i]);
        displacements[i] = static_cast<int>(totalBuf);
        totalBuf += allSizes[i];
        if ( totalBuf > static_cast<uint64_t>(std::numeric_limits<int>::max()) )
            throw std::length_error("Comms::all_gather: gathered data is too large for a single MPI_Allgatherv");
    }

    auto bigBuff = std::unique_ptr<char[]>(new char[totalBuf]);

    MPI_Allgatherv(buffer.data(), buffer.size(), MPI_BYTE, bigBuff.get(), counts.get(), displacements.get(), MPI_BYTE,
        MPI_COMM_WORLD);

    out_data.resize(world);
    for ( int i = 0; i < world; i++ ) {
        auto* bbuf = bigBuff.get();
        Comms::deserialize(&bbuf[displacements[i]], counts[i], out_data[i]);
    }
}

#endif

} // namespace SST::Comms

#endif // SST_CORE_OBJECTCOMMS_H
