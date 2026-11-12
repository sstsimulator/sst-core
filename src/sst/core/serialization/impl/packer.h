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

#ifndef SST_CORE_SERIALIZATION_IMPL_PACKER_H
#define SST_CORE_SERIALIZATION_IMPL_PACKER_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/packer.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include "sst/core/serialization/chunked_buffer.h"
#include "sst/core/serialization/impl/get_array_size.h"
#include "sst/core/serialization/impl/ser_buffer_accessor.h"
#include "sst/core/serialization/impl/ser_shared_ptr_tracker.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <type_traits>

namespace SST::Core::Serialization::pvt {

// Pointer tracking state used when packing, common to ser_packer and ser_packer_growable
class ser_pack_tracker : public ser_shared_ptr_packer
{
    std::set<uintptr_t> pointer_set;

public:
    bool check_pointer_pack(uintptr_t ptr) { return !pointer_set.insert(ptr).second; }
}; // class ser_pack_tracker

// Packing operations, implemented in terms of PACKER::write(const void* data, size_t size) and
// PACKER::write_value(const T& t)
template <typename PACKER>
class ser_pack_ops
{
    PACKER& packer() { return static_cast<PACKER&>(*this); }

public:
    template <typename T>
    void pack(const T& t)
    {
        packer().write_value(t);
    }

    template <typename T, typename SIZE_T>
    void pack_buffer(const T* buffer, SIZE_T size)
    {
        if ( buffer != nullptr )
            get_array_size(size, "Serialization Error: Size in SST::Core::Serialization:pvt::pack_buffer() cannot fit "
                                 "inside size_t. size_t should be used for sizes.\n");
        else
            size = 0;
        using ELEM_T = std::conditional_t<std::is_void_v<T>, char, T>; // Use char if T == void
        pack(size);
        packer().write(buffer, size * sizeof(ELEM_T));
    }

    // Defined in packer.cc, so that it is not inlined into serializer::string()
    void pack_string(const std::string& str);
}; // class ser_pack_ops

// Packer which writes to a single caller supplied buffer, which must be large enough to hold all of the data
class ser_packer : public ser_buffer_accessor, public ser_pack_tracker, public ser_pack_ops<ser_packer>
{
public:
    // inherit ser_buffer_accessor constructors
    using ser_buffer_accessor::ser_buffer_accessor;

    void write(const void* data, size_t size) { memcpy(buf_next(size), data, size); }

    template <typename T>
    void write_value(const T& t)
    {
        write(&t, sizeof(t));
    }
}; // class ser_packer

// Packer which writes to a chunked_buffer, allocating chunks as needed
class ser_packer_growable : public ser_pack_tracker, public ser_pack_ops<ser_packer_growable>
{
    chunked_buffer& buf_;
    char*           cur_ = nullptr; // next byte to write in the current chunk
    char*           end_ = nullptr; // end of the current chunk

    // Write data which does not fit in the current chunk
    void write_slow(const char* data, size_t size);

    // Write the first size bytes of word, which does not fit in the current chunk. Small values are passed in a
    // register instead of by address, so that callers' local variables do not need stack frames to hold them.
    void write_slow_word(uint64_t word, size_t size);

public:
    // Packing starts with an empty buffer. No chunk is allocated until data is written.
    explicit ser_packer_growable(chunked_buffer& buf) :
        buf_(buf)
    {
        buf_.clear();
    }

    ser_packer_growable(const ser_packer_growable&)            = delete;
    ser_packer_growable& operator=(const ser_packer_growable&) = delete;
    ~ser_packer_growable()                                     = default;

    void write(const void* data, size_t size)
    {
        if ( size <= static_cast<size_t>(end_ - cur_) ) {
            memcpy(cur_, data, size);
            cur_ += size;
            buf_.size_ += size;
        }
        else
            write_slow(static_cast<const char*>(data), size);
    }

    template <typename T>
    void write_value(const T& t)
    {
        if ( sizeof(T) <= static_cast<size_t>(end_ - cur_) ) {
            memcpy(cur_, &t, sizeof(T));
            cur_ += sizeof(T);
            buf_.size_ += sizeof(T);
        }
        else if constexpr ( sizeof(T) <= sizeof(uint64_t) ) {
            uint64_t word = 0;
            memcpy(&word, &t, sizeof(T));
            write_slow_word(word, sizeof(T));
        }
        else
            write_slow(reinterpret_cast<const char*>(&t), sizeof(T));
    }

    size_t size() const { return buf_.size(); }
}; // class ser_packer_growable

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_PACKER_H
