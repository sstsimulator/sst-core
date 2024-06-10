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

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_SERIALIZABLE_H
#define SST_CORE_SERIALIZATION_SERIALIZE_SERIALIZABLE_H

#include "sst/core/serialization/serializable.h"
#include "sst/core/serialization/serialize.h"
#include "sst/core/serialization/serializer.h"

#include <iostream>

namespace SST {
namespace Core {
namespace Serialization {

namespace pvt {

void size_serializable(serializable_base* s, serializer& ser);

void pack_serializable(serializable_base* s, serializer& ser);

void unpack_serializable(serializable_base*& s, serializer& ser);

} // namespace pvt


template <class T>
class serialize_impl<
    T*, typename std::enable_if<std::is_base_of<SST::Core::Serialization::serializable, T>::value>::type>
{

    template <class A>
    friend class serialize;
    void operator()(T*& s, serializer& ser)
    {
        serializable_base* sp = static_cast<serializable_base*>(s);
        switch ( ser.mode() ) {
        case serializer::SIZER:
            pvt::size_serializable(sp, ser);
            break;
        case serializer::PACK:
            pvt::pack_serializable(sp, ser);
            break;
        case serializer::UNPACK:
            pvt::unpack_serializable(sp, ser);
            break;
        }
        s = static_cast<T*>(sp);
    }
};

template <class T>
void
serialize_intrusive_ptr(T*& t, serializer& ser)
{
    serializable_base* s = t;
    switch ( ser.mode() ) {
    case serializer::SIZER:
        pvt::size_serializable(s, ser);
        break;
    case serializer::PACK:
        pvt::pack_serializable(s, ser);
        break;
    case serializer::UNPACK:
        pvt::unpack_serializable(s, ser);
        t = dynamic_cast<T*>(s);
        break;
    }
}

template <class T>
class serialize_impl<
    T, typename std::enable_if<std::is_base_of<SST::Core::Serialization::serializable, T>::value>::type>
{
    template <class A>
    friend class serialize;
    inline void operator()(T& t, serializer& ser)
    {
        // T* tmp = &t;
        // serialize_intrusive_ptr(tmp, ser);
        t.serialize_order(ser);
    }
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_SERIALIZE_SERIALIZABLE_H
