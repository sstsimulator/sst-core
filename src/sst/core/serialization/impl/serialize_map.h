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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_MAP_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_MAP_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_map.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <map>
#include <string>
#include <unordered_map>

namespace SST::Core::Serialization {

/**
   Class used to map std::map
 */

template <class Key, class Value>
class serialize_impl<std::map<Key, Value>>
{
    using Map = std::map<Key, Value>;

public:
    void operator()(Map& m, serializer& ser)
    {
        using iterator = typename std::map<Key, Value>::iterator;
        switch ( ser.mode() ) {

        case serializer::SIZER:
        {
            size_t size = m.size();
            ser.size(size);
            iterator it, end = m.end();
            for ( it = m.begin(); it != end; ++it ) {
                // keys are const values - annoyingly
                ser& const_cast<Key&>(it->first);
                ser & it->second;
            }
            break;
        }
        case serializer::PACK:
        {
            size_t size = m.size();
            ser.pack(size);
            iterator it, end = m.end();
            for ( it = m.begin(); it != end; ++it ) {
                ser& const_cast<Key&>(it->first);
                ser & it->second;
            }
            break;
        }
        case serializer::UNPACK:
        {
            size_t size;
            ser.unpack(size);
            for ( size_t i = 0; i < size; ++i ) {
                Key   k = {};
                Value v = {};
                ser&  k;
                ser&  v;
                m[k] = v;
            }
            break;
        }
        case serializer::MAP:
            // The version of function not called in mapping mode
            break;
        }
    }

    void operator()(Map& UNUSED(m), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Add support for mapping mode
    }
};

template <class Key, class Value>
class serialize<std::unordered_map<Key, Value>>
{
    using Map = std::unordered_map<Key, Value>;

public:
    void operator()(Map& m, serializer& ser)
    {
        using iterator = typename std::unordered_map<Key, Value>::iterator;
        switch ( ser.mode() ) {

        case serializer::SIZER:
        {
            size_t size = m.size();
            ser.size(size);
            iterator it, end = m.end();
            for ( it = m.begin(); it != end; ++it ) {
                // keys are const values - annoyingly
                ser& const_cast<Key&>(it->first);
                ser & it->second;
            }
            break;
        }
        case serializer::PACK:
        {
            size_t size = m.size();
            ser.pack(size);
            iterator it, end = m.end();
            for ( it = m.begin(); it != end; ++it ) {
                ser& const_cast<Key&>(it->first);
                ser & it->second;
            }
            break;
        }
        case serializer::UNPACK:
        {
            size_t size;
            ser.unpack(size);
            for ( size_t i = 0; i < size; ++i ) {
                Key   k = {};
                Value v = {};
                ser&  k;
                ser&  v;
                m[k] = v;
            }
            break;
        }
        case serializer::MAP:
            // The version of function not called in mapping mode
            break;
        }
    }

    void operator()(Map& UNUSED(m), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Add support for mapping mode
    }
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_MAP_H
