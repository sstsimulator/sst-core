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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ATOMIC_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_ATOMIC_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_atomic.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <atomic>

namespace SST::Core::Serialization {

template <class T>
class serialize_impl<std::atomic<T>>
{
    using Value = std::atomic<T>;

public:
    void operator()(Value& v, serializer& ser)
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        {
            T    t = v.load();
            ser& t;
            // ser.size(t);
            break;
        }
        case serializer::PACK:
        {
            T    t = v.load();
            ser& t;
            break;
        }
        case serializer::UNPACK:
        {
            T    val;
            ser& val;
            v.store(val);
            break;
        }
        case serializer::MAP:
            // The version of function not called in mapping mode
            break;
        }
    }

    void operator()(Value& UNUSED(v), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Add support for mapping mode
    }
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VECTOR_H
