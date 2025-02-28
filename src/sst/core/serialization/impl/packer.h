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

#include <string>

namespace SST::Core::Serialization::pvt {

class ser_packer : public ser_buffer_accessor
{
public:
    template <class T>
    void pack(T& t)
    {
        T* buf = ser_buffer_accessor::next<T>();
        DISABLE_WARN_MAYBE_UNINITIALIZED
        *buf = t;
        REENABLE_WARNING
    }

    /**
     * @brief pack_buffer
     * @param buf  Must be non-null
     * @param size Must be non-zero
     */
    void pack_buffer(void* buf, int size);

    void pack_string(std::string& str);
};

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_PACKER_H
