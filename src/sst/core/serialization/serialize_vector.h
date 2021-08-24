// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_VECTOR_H
#define SST_CORE_SERIALIZATION_SERIALIZE_VECTOR_H

#include "sst/core/serialization/serializer.h"

#include <vector>

namespace SST {
namespace Core {
namespace Serialization {

template <class T>
class serialize<std::vector<T>>
{
    typedef std::vector<T> Vector;

public:
    void operator()(Vector& v, serializer& ser)
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        {
            size_t size = v.size();
            ser.size(size);
            break;
        }
        case serializer::PACK:
        {
            size_t size = v.size();
            ser.pack(size);
            break;
        }
        case serializer::UNPACK:
        {
            size_t s;
            ser.unpack(s);
            v.resize(s);
            break;
        }
        }

        for ( size_t i = 0; i < v.size(); ++i ) {
            serialize<T>()(v[i], ser);
        }
    }
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_SERIALIZE_VECTOR_H
