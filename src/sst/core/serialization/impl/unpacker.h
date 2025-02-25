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

#ifndef SST_CORE_SERIALIZATION_IMPL_UNPACKER_H
#define SST_CORE_SERIALIZATION_IMPL_UNPACKER_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/unpacker.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include "sst/core/serialization/impl/ser_buffer_accessor.h"

namespace SST::Core::Serialization::pvt {

class ser_unpacker : public ser_buffer_accessor
{
public:
    template <class T>
    void unpack(T& t)
    {
        T* bufptr = ser_buffer_accessor::next<T>();
        t         = *bufptr;
    }

    /**
     * @brief unpack_buffer
     * @param buf   Must unpack to non-null buffer
     * @param size  Must be non-zero
     */
    void unpack_buffer(void* buf, int size);

    void unpack_string(std::string& str);
};

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_UNPACKER_H
