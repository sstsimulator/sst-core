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

#ifndef SST_CORE_SERIALIZATION_SERIALIZABLE_H
#define SST_CORE_SERIALIZATION_SERIALIZABLE_H

#include "sst/core/serialization/serializable_base.h"
#include "sst/core/serialization/serialize.h"

namespace SST::Core::Serialization {

class serializable : public serializable_base
{
public:
    static constexpr uint32_t NullClsId = std::numeric_limits<uint32_t>::max();

    // virtual const char* cls_name() const = 0;

    // virtual void serialize_order(serializer& ser) = 0;

    // virtual uint32_t    cls_id() const             = 0;
    // virtual std::string serialization_name() const = 0;

    virtual ~serializable() {}
};

namespace pvt {

void size_serializable(serializable_base* s, serializer& ser);

void pack_serializable(serializable_base* s, serializer& ser);

void unpack_serializable(serializable_base*& s, serializer& ser);

void map_serializable(serializable_base*& s, serializer& ser, const char* name);

} // namespace pvt


template <class T>
class serialize_impl<T*, std::enable_if_t<std::is_base_of_v<SST::Core::Serialization::serializable, T>>>
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
        case serializer::MAP:
            // No mapping without a name
            break;
        }
        s = static_cast<T*>(sp);
    }

    void operator()(T*& s, serializer& ser, const char* name)
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
        case serializer::MAP:
            pvt::map_serializable(sp, ser, name);
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
    case serializer::MAP:
        // Add your code here
        break;
    }
}

template <class T>
class serialize_impl<T, std::enable_if_t<std::is_base_of_v<SST::Core::Serialization::serializable, T>>>
{
    template <class A>
    friend class serialize;
    inline void operator()(T& t, serializer& ser)
    {
        // T* tmp = &t;
        // serialize_intrusive_ptr(tmp, ser);
        t.serialize_order(ser);
    }

    inline void operator()(T& UNUSED(t), serializer& UNUSED(ser), const char* UNUSED(name))
    {
        // TODO: Need to figure out how to handle mapping mode for
        // classes inheriting from serializable that are not pointers.
        // For the core, this really only applies to SharedObjects,
        // which may actually need their own specific serialization.

        // For now mapping mode won't provide any data
    }
};

} // namespace SST::Core::Serialization

//#include "sst/core/serialization/serialize_serializable.h"

#endif
