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

#ifndef SST_CORE_SERIALIZATION_CHUNKED_BUFFER_H
#define SST_CORE_SERIALIZATION_CHUNKED_BUFFER_H

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <vector>

namespace SST::Core::Serialization {

namespace pvt {
class ser_packer_growable;
} // namespace pvt

/**
 * Buffer made of chunks which grow as needed, used with serializer::start_packing(chunked_buffer&) and
 * serializer::start_unpacking(const chunked_buffer&).
 *
 * Packing into a chunked_buffer does not need a sizing pass. A chunk is allocated with min_chunk_size() bytes and grows
 * (doubling, and copying the data it already holds) until it reaches max_chunk_size() bytes. When a chunk is full at
 * max_chunk_size(), the data continues in a new chunk, which again starts at min_chunk_size() and grows as needed.
 *
 * The data is stored densely: chunk i holds bytes [i * max_chunk_size(), (i + 1) * max_chunk_size()) of the serialized
 * data, and only the last chunk may be partially filled. Concatenating the chunks gives exactly the bytes that
 * packing into a single buffer with serializer::start_packing(char*, size_t) would have produced, so data packed one
 * way can be unpacked the other way. When the data fits in max_chunk_size() bytes, it is in a single contiguous chunk
 * which can be used directly, for example as an MPI message, without copying it.
 *
 * The chunked_buffer is owned by the caller. clear() discards the data but keeps the allocated chunks at their grown
 * sizes, so the cost of growing is only paid when a serialization is larger than any before it. A chunked_buffer must
 * not be modified or destroyed while a serializer is packing into it or unpacking from it.
 */
class chunked_buffer
{
public:
    // Default size of a newly allocated chunk, limited to max_chunk_size
    static constexpr size_t default_min_chunk_size = 4096;

    explicit chunked_buffer(size_t max_chunk_size, size_t min_chunk_size = default_min_chunk_size);

    chunked_buffer(const chunked_buffer&)            = delete;
    chunked_buffer& operator=(const chunked_buffer&) = delete;
    chunked_buffer(chunked_buffer&&)                 = default;
    chunked_buffer& operator=(chunked_buffer&&)      = default;
    ~chunked_buffer()                                = default;

    // Total number of bytes of data in the buffer
    size_t size() const { return size_; }

    // Size that a chunk grows to before the data continues in another chunk
    size_t max_chunk_size() const { return max_chunk_size_; }

    // Size of a newly allocated chunk
    size_t min_chunk_size() const { return min_chunk_size_; }

    // Number of chunks that hold data
    size_t num_chunks() const { return (size_ + max_chunk_size_ - 1) / max_chunk_size_; }

    // Data of chunk i, and the number of bytes of data it holds (i < num_chunks())
    char*       chunk_data(size_t i) { return chunks_[i].data.get(); }
    const char* chunk_data(size_t i) const { return chunks_[i].data.get(); }
    size_t      chunk_bytes(size_t i) const { return std::min(max_chunk_size_, size_ - i * max_chunk_size_); }

    // Total number of bytes allocated for all chunks, including chunks which do not currently hold data
    size_t capacity() const;

    // Discard the data, keeping the allocated chunks for reuse
    void clear() { size_ = 0; }

    // Discard the data and free all of the chunks. New chunks are allocated at min_chunk_size() when they are needed.
    void release();

    // Set the size of the data to size bytes, allocating or growing chunks as needed. This is used to fill the chunks
    // from another source (file, network) before unpacking. Chunks which are already large enough for the data they
    // hold after resizing keep their contents. Chunks which have to grow are reallocated without keeping their
    // contents.
    void resize(size_t size);

    // Copy the data to a contiguous buffer which must hold at least size() bytes
    void copy_to(char* dst) const;

    // Return a copy of the data in a contiguous std::vector
    std::vector<char> to_vector() const;

    // Write the data to an output stream, one chunk at a time
    void write_to(std::ostream& os) const;

private:
    friend class pvt::ser_packer_growable;

    struct chunk
    {
        std::unique_ptr<char[]> data;
        size_t                  capacity = 0;
    };

    // Make room to write at least one, and up to needed, bytes at offset size() of the data, allocating or growing
    // the chunk holding that offset. Returns a pointer to that offset and sets end to the end of the chunk's capacity.
    char* make_space(size_t needed, char*& end);

    // Reallocate chunk c with capacity bytes, keeping its first keep bytes
    void reallocate(chunk& c, size_t capacity, size_t keep);

    size_t             max_chunk_size_;
    size_t             min_chunk_size_;
    size_t             size_ = 0;
    std::vector<chunk> chunks_;
}; // class chunked_buffer

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_CHUNKED_BUFFER_H
