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

#ifndef SST_CORE_SERIALIZATION_IMPL_UNPACKER_H
#define SST_CORE_SERIALIZATION_IMPL_UNPACKER_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/unpacker.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include "sst/core/serialization/chunked_buffer.h"
#include "sst/core/serialization/impl/ser_buffer_accessor.h"
#include "sst/core/serialization/impl/ser_shared_ptr_tracker.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <type_traits>

namespace SST::Core::Serialization::pvt {

// Pointer tracking state used when unpacking, common to ser_unpacker and ser_unpacker_growable
class ser_unpack_tracker : public ser_shared_ptr_unpacker
{
    std::map<uintptr_t, uintptr_t> ser_pointer_map;
    uintptr_t                      split_key = 0;

public:
    uintptr_t check_pointer_unpack(uintptr_t ptr);
    void      report_new_pointer(uintptr_t real_ptr) { ser_pointer_map[split_key] = real_ptr; }
    void      report_real_pointer(uintptr_t ptr, uintptr_t real_ptr) { ser_pointer_map[ptr] = real_ptr; }
}; // class ser_unpack_tracker

// Unpacking operations, implemented in terms of UNPACKER::read(void* data, size_t size) and
// UNPACKER::read_value(T& t)
template <typename UNPACKER>
class ser_unpack_ops
{
    UNPACKER& unpacker() { return static_cast<UNPACKER&>(*this); }

public:
    template <typename T>
    void unpack(T& t)
    {
        unpacker().read_value(t);
    }

    template <typename T, typename SIZE_T>
    void unpack_buffer(T*& buffer, SIZE_T& size)
    {
        size = 0;
        unpack(size);
        if ( size != 0 ) {
            using ELEM_T = std::conditional_t<std::is_void_v<T>, char, T>; // Use char if T == void
            buffer       = new ELEM_T[size];
            unpacker().read(buffer, size * sizeof(ELEM_T));
        }
        else
            buffer = nullptr;
    }

    // Defined in unpacker.cc, so that its stack frame is not inlined into serializer::string()
    void unpack_string(std::string& str);
}; // class ser_unpack_ops

// Unpacker which reads from a single contiguous buffer
class ser_unpacker : public ser_buffer_accessor, public ser_unpack_tracker, public ser_unpack_ops<ser_unpacker>
{
public:
    // inherit ser_buffer_accessor constructors
    using ser_buffer_accessor::ser_buffer_accessor;

    void read(void* data, size_t size) { memcpy(data, buf_next(size), size); }

    template <typename T>
    void read_value(T& t)
    {
        read(&t, sizeof(t));
    }
}; // class ser_unpacker

// Unpacker which reads from a chunked_buffer
class ser_unpacker_growable : public ser_unpack_tracker, public ser_unpack_ops<ser_unpacker_growable>
{
    const chunked_buffer& buf_;
    size_t                chunk_ = 0;       // index of the current chunk
    size_t                size_  = 0;       // number of bytes read so far
    const char*           cur_   = nullptr; // next byte to read in the current chunk
    const char*           end_   = nullptr; // end of the data in the current chunk

    // Read data which is not all in the current chunk
    void read_slow(char* data, size_t size);

    // Read size bytes which are not all in the current chunk, returning them in the first size bytes of a word. Small
    // values are returned in a register instead of by address, so that callers' local variables do not need stack
    // frames to hold them.
    uint64_t read_slow_word(size_t size);

public:
    explicit ser_unpacker_growable(const chunked_buffer& buf) :
        buf_(buf)
    {
        if ( buf_.num_chunks() != 0 ) {
            cur_ = buf_.chunk_data(0);
            end_ = cur_ + buf_.chunk_bytes(0);
        }
    }

    ser_unpacker_growable(const ser_unpacker_growable&)            = delete;
    ser_unpacker_growable& operator=(const ser_unpacker_growable&) = delete;
    ~ser_unpacker_growable()                                       = default;

    void read(void* data, size_t size)
    {
        if ( size <= static_cast<size_t>(end_ - cur_) ) {
            memcpy(data, cur_, size);
            cur_ += size;
            size_ += size;
        }
        else
            read_slow(static_cast<char*>(data), size);
    }

    template <typename T>
    void read_value(T& t)
    {
        if ( sizeof(T) <= static_cast<size_t>(end_ - cur_) ) {
            memcpy(static_cast<void*>(&t), cur_, sizeof(T));
            cur_ += sizeof(T);
            size_ += sizeof(T);
        }
        else if constexpr ( sizeof(T) <= sizeof(uint64_t) ) {
            uint64_t word = read_slow_word(sizeof(T));
            memcpy(static_cast<void*>(&t), &word, sizeof(T));
        }
        else
            read_slow(reinterpret_cast<char*>(&t), sizeof(T));
    }

    size_t size() const { return size_; }
}; // class ser_unpacker_growable

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_UNPACKER_H
