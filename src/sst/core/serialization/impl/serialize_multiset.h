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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_MULTISET_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_MULTISET_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_multiset.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <set>
#include <unordered_set>

namespace SST::Core::Serialization {

template <class T, class C>
class serialize<std::multiset<T, C>>
{
    typedef std::multiset<T, C> MultiSet;

public:
    void operator()(MultiSet& v, serializer& ser)
    {
        typedef typename std::multiset<T>::iterator iterator;
        switch ( ser.mode() ) {
        case serializer::SIZER:
        {
            size_t size = v.size();
            ser.size(size);
            iterator it, end = v.end();
            for ( it = v.begin(); it != end; ++it ) {
                T&   t = const_cast<T&>(*it);
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
                T&   t = const_cast<T&>(*it);
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
                v.insert(t);
            }
            break;
        }
        case serializer::MAP:
            // The version of function not called in mapping mode
            break;
        }
    }

    void operator()(MultiSet& UNUSED(v), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Add support for mapping mode
    }
};

template <class T>
class serialize<std::unordered_multiset<T>>
{
    typedef std::unordered_multiset<T> MultiSet;

public:
    void operator()(MultiSet& v, serializer& ser)
    {
        typedef typename std::unordered_multiset<T>::iterator iterator;
        switch ( ser.mode() ) {
        case serializer::SIZER:
        {
            size_t size = v.size();
            ser.size(size);
            iterator it, end = v.end();
            for ( it = v.begin(); it != end; ++it ) {
                T&   t = const_cast<T&>(*it);
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
                T&   t = const_cast<T&>(*it);
                ser& t;
            }
            break;
        }
        case serializer::UNPACK:
        {
            size_t size;
            ser.unpack(size);
            for ( size_t i = 0; i < size; ++i ) {
                T    t = {};
                ser& t;
                v.insert(t);
            }
            break;
        }
        case serializer::MAP:
            // The version of function not called in mapping mode
            break;
        }
    }

    void operator()(MultiSet& UNUSED(v), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Add support for mapping mode
    }
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_MULTISET_H
