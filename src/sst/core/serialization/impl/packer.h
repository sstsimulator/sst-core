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

#include "sst/core/serialization/impl/ser_buffer_accessor.h"

#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <type_traits>

namespace SST::Core::Serialization::pvt {

class ser_packer : public ser_buffer_accessor
{
    std::set<uintptr_t> pointer_set;

public:
    // inherit ser_buffer_accessor constructors
    using ser_buffer_accessor::ser_buffer_accessor;

    template <typename T>
    void pack(T&& t)
    {
        memcpy(buf_next(sizeof(t)), &t, sizeof(t));
    }

    template <typename ELEM_T, typename SIZE_T>
    void pack_buffer(ELEM_T* buffer, SIZE_T size)
    {
        if ( buffer == nullptr ) size = 0;
        pack(size);
        if constexpr ( std::is_void_v<ELEM_T> )
            memcpy(buf_next(size), buffer, size);
        else
            memcpy(buf_next(size * sizeof(ELEM_T)), buffer, size * sizeof(ELEM_T));
    }

    void pack_string(std::string& str);
    bool check_pointer_pack(uintptr_t ptr) { return !pointer_set.insert(ptr).second; }
}; // class ser_packer

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_PACKER_H
