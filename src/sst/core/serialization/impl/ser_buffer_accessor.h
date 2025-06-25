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

#ifndef SST_CORE_SERIALIZATION_IMPL_SER_BUFFER_ACCESSOR_H
#define SST_CORE_SERIALIZATION_IMPL_SER_BUFFER_ACCESSOR_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/ser_buffer_accessor.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include <cstddef>
#include <stdexcept>

namespace SST::Core::Serialization::pvt {

class ser_buffer_accessor
{
    char* const  bufstart_;
    size_t const max_size_;
    char*        bufptr_ = bufstart_;
    size_t       size_   = 0;

public:
    // constructor which is inherited by packer and unpacker
    ser_buffer_accessor(void* buffer, size_t size) :
        bufstart_(static_cast<char*>(buffer)),
        max_size_(size)
    {}

    ser_buffer_accessor(const ser_buffer_accessor&)            = delete;
    ser_buffer_accessor& operator=(const ser_buffer_accessor&) = delete;
    ~ser_buffer_accessor()                                     = default;

    // return a pointer to the buffer and then advance it size bytes
    void* buf_next(size_t size)
    {
        size_ += size;
        if ( size_ > max_size_ ) throw std::out_of_range("serialization buffer overrun");
        char* buf = bufptr_;
        bufptr_ += size;
        return buf;
    }

    size_t size() const { return size_; }
};

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_SER_BUFFER_ACCESSOR_H
