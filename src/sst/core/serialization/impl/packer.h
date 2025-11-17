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

#ifndef SST_CORE_SERIALIZATION_IMPL_PACKER_H
#define SST_CORE_SERIALIZATION_IMPL_PACKER_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/packer.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include "sst/core/serialization/impl/get_array_size.h"
#include "sst/core/serialization/impl/ser_buffer_accessor.h"
#include "sst/core/serialization/impl/ser_shared_ptr_tracker.h"

#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <type_traits>

namespace SST::Core::Serialization::pvt {

class ser_packer : public ser_buffer_accessor, public ser_shared_ptr_packer
{
    std::set<uintptr_t> pointer_set;

public:
    // inherit ser_buffer_accessor constructors
    using ser_buffer_accessor::ser_buffer_accessor;

    template <typename T>
    void pack(const T& t)
    {
        memcpy(buf_next(sizeof(t)), &t, sizeof(t));
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
        memcpy(buf_next(size * sizeof(ELEM_T)), buffer, size * sizeof(ELEM_T));
    }

    void pack_string(const std::string& str) { pack_buffer(str.data(), str.size()); }
    bool check_pointer_pack(uintptr_t ptr) { return !pointer_set.insert(ptr).second; }
}; // class ser_packer

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_PACKER_H
