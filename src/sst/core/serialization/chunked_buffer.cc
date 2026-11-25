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

#include "sst/core/serialization/chunked_buffer.h"

#include "sst/core/serialization/serializer.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <stdexcept>

namespace SST::Core::Serialization {

chunked_buffer::chunked_buffer(size_t max_chunk_size, size_t min_chunk_size) :
    max_chunk_size_(max_chunk_size),
    min_chunk_size_(std::min(min_chunk_size, max_chunk_size))
{
    if ( max_chunk_size == 0 || min_chunk_size == 0 )
        throw std::invalid_argument("chunked_buffer: chunk sizes must be greater than 0");
}

void
chunked_buffer::reallocate(chunk& c, size_t capacity, size_t keep)
{
    // Chunks are not value-initialized, since they are always written before they are read
    std::unique_ptr<char[]> data(new char[capacity]);
    if ( keep != 0 ) memcpy(data.get(), c.data.get(), keep);
    c.data     = std::move(data);
    c.capacity = capacity;
}

char*
chunked_buffer::make_space(size_t needed, char*& end)
{
    size_t index  = size_ / max_chunk_size_;
    size_t offset = size_ % max_chunk_size_;

    // A new chunk starts at min_chunk_size_, or larger if needed is larger, so that large writes are not copied
    // while the chunk grows
    if ( index == chunks_.size() ) {
        chunks_.emplace_back();
        reallocate(chunks_.back(), std::clamp(needed, min_chunk_size_, max_chunk_size_), 0);
    }

    // An existing chunk which is full grows to the larger of double its capacity and what is needed, up to
    // max_chunk_size_, keeping the data it holds
    chunk& c = chunks_[index];
    if ( offset == c.capacity )
        reallocate(c, std::min(max_chunk_size_, std::max(2 * c.capacity, offset + needed)), offset);

    end = c.data.get() + c.capacity;
    return c.data.get() + offset;
}

void
chunked_buffer::resize(size_t size)
{
    size_    = size;
    size_t n = num_chunks();
    if ( chunks_.size() < n ) chunks_.resize(n);

    // Each chunk needs room for the data it holds. Chunks which grow do not keep their contents.
    for ( size_t i = 0; i < n; ++i ) {
        size_t bytes = chunk_bytes(i);
        if ( chunks_[i].data == nullptr || chunks_[i].capacity < bytes )
            reallocate(chunks_[i], std::max(bytes, min_chunk_size_), 0);
    }
}

void
chunked_buffer::release()
{
    std::vector<chunk>().swap(chunks_);
    size_ = 0;
}

size_t
chunked_buffer::capacity() const
{
    size_t total = 0;
    for ( auto& c : chunks_ )
        total += c.capacity;
    return total;
}

void
chunked_buffer::copy_to(char* dst) const
{
    for ( size_t i = 0, n = num_chunks(); i < n; ++i ) {
        memcpy(dst, chunk_data(i), chunk_bytes(i));
        dst += chunk_bytes(i);
    }
}

std::vector<char>
chunked_buffer::to_vector() const
{
    std::vector<char> vec(size_);
    copy_to(vec.data());
    return vec;
}

void
chunked_buffer::write_to(std::ostream& os) const
{
    for ( size_t i = 0, n = num_chunks(); i < n; ++i )
        os.write(chunk_data(i), chunk_bytes(i));
}

namespace pvt {

void
ser_packer_growable::write_slow(const char* data, size_t size)
{
    // Fill the rest of the current chunk, then grow it or move to the next chunk, until the remaining data fits
    while ( size > static_cast<size_t>(end_ - cur_) ) {
        size_t avail = end_ - cur_;
        if ( avail != 0 ) {
            memcpy(cur_, data, avail);
            data += avail;
            size -= avail;
            buf_.size_ += avail;
        }
        cur_ = buf_.make_space(size, end_);
    }
    memcpy(cur_, data, size);
    cur_ += size;
    buf_.size_ += size;
}

void
ser_packer_growable::write_slow_word(uint64_t word, size_t size)
{
    write_slow(reinterpret_cast<const char*>(&word), size);
}

void
ser_unpacker_growable::read_slow(char* data, size_t size)
{
    if ( size > buf_.size() - size_ ) throw std::out_of_range("serialization buffer overrun");

    // Copy the rest of the current chunk and move to the next one until the remaining data is in the current chunk.
    // The check above guarantees that chunk_ stays below buf_.num_chunks().
    while ( size > static_cast<size_t>(end_ - cur_) ) {
        size_t avail = end_ - cur_;
        memcpy(data, cur_, avail);
        data += avail;
        size -= avail;
        size_ += avail;
        ++chunk_;
        cur_ = buf_.chunk_data(chunk_);
        end_ = cur_ + buf_.chunk_bytes(chunk_);
    }
    memcpy(data, cur_, size);
    cur_ += size;
    size_ += size;
}

uint64_t
ser_unpacker_growable::read_slow_word(size_t size)
{
    uint64_t word = 0;
    read_slow(reinterpret_cast<char*>(&word), size);
    return word;
}

} // namespace pvt

} // namespace SST::Core::Serialization
