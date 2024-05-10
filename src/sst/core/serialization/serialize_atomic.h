// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_ATOMIC_H
#define SST_CORE_SERIALIZATION_SERIALIZE_ATOMIC_H

#include "sst/core/serialization/serializer.h"

#include <atomic>

namespace SST {
namespace Core {
namespace Serialization {

template <class T>
class serialize<std::atomic<T>>
{
    typedef std::atomic<T> Value;

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
        }
    }
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_SERIALIZE_VECTOR_H
