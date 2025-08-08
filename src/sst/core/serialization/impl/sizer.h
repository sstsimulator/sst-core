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

#ifndef SST_CORE_SERIALIZATION_IMPL_SIZER_H
#define SST_CORE_SERIALIZATION_IMPL_SIZER_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/sizer.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include "sst/core/serialization/impl/get_array_size.h"
#include "sst/core/serialization/impl/ser_shared_ptr_tracker.h"

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <type_traits>

namespace SST::Core::Serialization::pvt {

class ser_sizer : public ser_shared_ptr_packer
{
    size_t              size_ = 0;
    std::set<uintptr_t> pointer_set;

public:
    explicit ser_sizer() = default;

    template <typename T>
    void size(const T&)
    {
        size_ += sizeof(T);
    }

    template <typename T, typename SIZE_T>
    void size_buffer(const T* buffer, SIZE_T size)
    {
        if ( buffer != nullptr )
            get_array_size(size, "Serialization Error: Size in SST::Core::Serialization:pvt::size_buffer() cannot fit "
                                 "inside size_t. size_t should be used for sizes.\n");
        else
            size = 0;
        using ELEM_T = std::conditional_t<std::is_void_v<T>, char, T>; // Use char if T == void
        size_ += sizeof(size) + size * sizeof(ELEM_T);
    }

    void   size_string(std::string& str) { size_ += sizeof(size_t) + str.size(); }
    void   add(size_t s) { size_ += s; }
    size_t size() const { return size_; }
    bool   check_pointer_sizer(uintptr_t ptr) { return !pointer_set.insert(ptr).second; }
}; // class ser_sizer

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_SIZER_H
