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

#include "sst_config.h"

#include "sst/core/sync/syncQueue.h"

#include "sst/core/event.h"
#include "sst/core/objectComms.h"
#include "sst/core/output.h"
#include "sst/core/profile/syncProfileTool.h"
#include "sst/core/serialization/serializer.h"
#include "sst/core/simulation.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <mutex>

namespace SST {

using namespace Core::ThreadSafe;
using namespace Core::Serialization;

RankSyncQueue::RankSyncQueue(RankInfo to_rank) :
    SyncQueue(to_rank),
    buffer(Comms::getMaxMessageSize())
{}

bool
RankSyncQueue::empty()
{
    std::scoped_lock lock(slock);
    return activities.empty();
}

int
RankSyncQueue::size()
{
    std::scoped_lock lock(slock);
    return activities.size();
}

void
RankSyncQueue::insert(Activity* activity)
{
    std::scoped_lock lock(slock);
    activities.push_back(activity);
}

Activity*
RankSyncQueue::pop()
{
    // NEED TO FATAL
    // if ( data.size() == 0 ) return nullptr;
    // std::vector<Activity*>::iterator it = data.begin();
    // Activity* ret_val = (*it);
    // data.erase(it);
    // return ret_val;
    return nullptr;
}

Activity*
RankSyncQueue::front()
{
    // NEED TO FATAL
    return nullptr;
}

void
RankSyncQueue::clear()
{
    std::scoped_lock lock(slock);
    activities.clear();
}

chunked_buffer&
RankSyncQueue::getData()
{
    std::scoped_lock lock(slock);

    serializer ser;

    ser.start_packing(buffer);

    // Reserve space for the header, which is filled in once the size is known
    Header hdr {};
    ser.primitive(hdr);

    SST_SER(activities);

    size_t size = buffer.size();

    if ( profile_tools_ ) profile_tools_->updateSyncSize(size - sizeof(Header), activities.size());

    // Delete all the events
    for ( unsigned int i = 0; i < activities.size(); i++ ) {
        delete activities[i];
    }
    activities.clear();

    // Set the size fields in the header, which is at the start of the first chunk
    hdr.buffer_size = size;
    hdr.chunk_size  = buffer.max_chunk_size();
    memcpy(buffer.chunk_data(0), &hdr, sizeof(hdr));

    return buffer;
}

void
RankSyncQueue::releaseBuffer()
{
    std::scoped_lock lock(slock);
    buffer.release();
}

#ifdef SST_CONFIG_HAVE_MPI
void
RankSyncQueue::sendData(int dest, int tag, int follow_tag, size_t& remote_size, std::vector<MPI_Request>& reqs)
{
    Header* hdr        = reinterpret_cast<Header*>(buffer.chunk_data(0));
    size_t  first_size = buffer.chunk_bytes(0);

    if ( first_size <= remote_size ) {
        // The first chunk fits in the remote buffer, so send it as the first message
        hdr->mode = 0;
        MPI_Isend(buffer.chunk_data(0), static_cast<int>(first_size), MPI_BYTE, dest, tag, MPI_COMM_WORLD,
            &reqs.emplace_back());
    }
    else {
        // Not big enough, so send just the header to tell the remote side to grow its buffer, followed by the first
        // chunk using follow_tag
        hdr->mode = 1;
        MPI_Isend(buffer.chunk_data(0), static_cast<int>(sizeof(Header)), MPI_BYTE, dest, tag, MPI_COMM_WORLD,
            &reqs.emplace_back());
        MPI_Isend(buffer.chunk_data(0), static_cast<int>(first_size), MPI_BYTE, dest, follow_tag, MPI_COMM_WORLD,
            &reqs.emplace_back());
        remote_size = first_size;
    }

    // Send the remaining chunks
    for ( size_t i = 1; i < buffer.num_chunks(); ++i )
        MPI_Isend(buffer.chunk_data(i), static_cast<int>(buffer.chunk_bytes(i)), MPI_BYTE, dest, follow_tag,
            MPI_COMM_WORLD, &reqs.emplace_back());
}
#endif

RankSyncRecvBuffer::RankSyncRecvBuffer() :
    buffer(Comms::getMaxMessageSize()),
    first_size(initialSize())
{
    // Allocate the first chunk, which receives the first message
    buffer.resize(first_size);
}

void
RankSyncRecvBuffer::reset()
{
    buffer.release();

    // Allocate the first chunk, which receives the first message, as in the constructor
    first_size = initialSize();
    buffer.resize(first_size);
}

size_t
RankSyncRecvBuffer::initialSize()
{
    return std::min<size_t>(4096, Comms::getMaxMessageSize());
}

#ifdef SST_CONFIG_HAVE_MPI
void
RankSyncRecvBuffer::postFirstRecv(int src, int tag, MPI_Request* req)
{
    // The first chunk always has room for first_size bytes
    MPI_Irecv(buffer.chunk_data(0), static_cast<int>(first_size), MPI_BYTE, src, tag, MPI_COMM_WORLD, req);
}

void
RankSyncRecvBuffer::postRemainingRecvs(int src, int follow_tag, std::vector<MPI_Request>& reqs)
{
    RankSyncQueue::Header hdr;
    memcpy(&hdr, buffer.chunk_data(0), sizeof(hdr));

    // The chunks must have the same layout on both ranks. Unlike Comms::recv(), the receive buffer is kept between
    // transfers and is not rebuilt for the sender's chunk size, so every rank must use the same mpi-max-message-size.
    // This is ensured by setting it after the Config has been broadcast.
    if ( hdr.chunk_size != buffer.max_chunk_size() )
        Output::getDefaultObject().fatal(CALL_INFO, 1,
            "ERROR: rank %d sends MPI messages of up to %" PRIu64 " bytes, but this rank uses %zu bytes. "
            "The mpi-max-message-size option must be the same on all ranks.\n",
            src, hdr.chunk_size, buffer.max_chunk_size());

    // In mode 0, the first chunk already holds the first message, which fits in its capacity, so resizing keeps it. In
    // mode 1, the first chunk grows and is received again with the remaining chunks, and the sender now knows that the
    // first message can be as large as the first chunk.
    buffer.resize(hdr.buffer_size);

    size_t first_chunk = 1;
    if ( hdr.mode == 1 ) {
        first_size  = buffer.chunk_bytes(0);
        first_chunk = 0;
    }

    for ( size_t i = first_chunk; i < buffer.num_chunks(); ++i )
        MPI_Irecv(buffer.chunk_data(i), static_cast<int>(buffer.chunk_bytes(i)), MPI_BYTE, src, follow_tag,
            MPI_COMM_WORLD, &reqs.emplace_back());
}
#endif

void
RankSyncRecvBuffer::unpack(std::vector<Activity*>& activities)
{
    serializer ser;
    ser.start_unpacking(buffer);

    // Skip the header
    RankSyncQueue::Header hdr;
    ser.primitive(hdr);

    SST_SER(activities);
}

} // namespace SST
