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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VECTOR_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VECTOR_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_vector.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <string>
#include <vector>

namespace SST {
namespace Core {
namespace Serialization {


/**
   Class used to map std::vectors.
 */
template <class T>
class ObjectMapVector : public ObjectMapWithChildren
{
protected:
    std::vector<T>* addr_;

public:
    bool isContainer() override { return true; }

    std::string getType() override { return demangle_name(typeid(std::vector<T>).name()); }

    void* getAddr() override { return addr_; }

    ObjectMapVector(std::vector<T>* addr) : ObjectMapWithChildren(), addr_(addr) {}

    ~ObjectMapVector() {}
};


template <class T>
class serialize_impl<std::vector<T>>
{
    template <class A>
    friend class serialize;

    typedef std::vector<T> Vector;

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
        case serializer::MAP:
            // If this version of operator() is called during mapping
            // mode, then the variable being mapped did not provide a
            // name, which means no ObjectMap will be created.
            break;
        }

        for ( size_t i = 0; i < v.size(); ++i ) {
            ser& v[i];
        }
    }

    void operator()(Vector& v, serializer& ser, const char* name)
    {
        if ( ser.mode() != serializer::MAP ) return operator()(v, ser);

        ObjectMapVector<T>* obj_map = new ObjectMapVector<T>(&v);
        ser.mapper().map_hierarchy_start(name, obj_map);
        for ( size_t i = 0; i < v.size(); ++i ) {
            sst_map_object(ser, v[i], std::to_string(i).c_str());
        }
        ser.mapper().map_hierarchy_end();
    }
};

template <>
class serialize_impl<std::vector<bool>>
{
    template <class A>
    friend class serialize;

    typedef std::vector<bool> Vector;

    void operator()(Vector& v, serializer& ser)
    {
        switch ( ser.mode() ) {
        case serializer::SIZER:
        {
            size_t size = v.size();
            ser.size(size);
            bool val;
            for ( auto it = v.begin(); it != v.end(); it++ ) {
                val = *it;
                ser& val;
            }
            break;
        }
        case serializer::PACK:
        {
            size_t size = v.size();
            ser.pack(size);
            for ( auto it = v.begin(); it != v.end(); it++ ) {
                bool val = *it;
                ser& val;
            }
            break;
        }
        case serializer::UNPACK:
        {
            size_t s;
            ser.unpack(s);
            v.resize(s);
            for ( size_t i = 0; i < v.size(); i++ ) {
                bool val = false;
                ser& val;
                v[i] = val;
            }
            break;
        }
        case serializer::MAP:
            // If this version of operator() is called during mapping
            // mode, then the variable being mapped did not provide a
            // name, which means no ObjectMap will be created.
            break;
        }
    }

    // TODO: Add support for mapping vector<bool>.  The weird way they
    // pack the bits means we'll likely need to have a special case of
    // ObjectMapVector<bool> that knows how to handle the packing.
};


} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VECTOR_H
