// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_LIST_H
#define SST_CORE_SERIALIZATION_SERIALIZE_LIST_H

#include "sst/core/serialization/serializer.h"

#include <list>

namespace SST {
namespace Core {
namespace Serialization {

template <class T>
class serialize<std::list<T>>
{
    typedef std::list<T> List;

public:
    void operator()(List& v, serializer& ser)
    {
        typedef typename List::iterator iterator;
        switch ( ser.mode() ) {
        case serializer::SIZER:
        {
            size_t size = v.size();
            ser.size(size);
            iterator it, end = v.end();
            for ( it = v.begin(); it != end; ++it ) {
                T&   t = *it;
                ser& t;
            }
            break;
        }
        case serializer::PACK:
        {
            size_t size = v.size();
            ser.pack(size);
            iterator it, end = v.end();
            for ( it = v.begin(); it != end; ++it ) {
                T&   t = *it;
                ser& t;
            }
            break;
        }
        case serializer::UNPACK:
        {
            size_t size;
            ser.unpack(size);
            for ( size_t i = 0; i < size; ++i ) {
                T    t;
                ser& t;
                v.push_back(t);
            }
            break;
        }
        }
    }
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_SERIALIZE_LIST_H
