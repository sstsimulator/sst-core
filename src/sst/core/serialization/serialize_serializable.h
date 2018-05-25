// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SERIALIZE_SERIALIZABLE_H
#define SERIALIZE_SERIALIZABLE_H

#include <sst/core/serialization/serializable.h>
#include <sst/core/serialization/serializer.h>
#include <sst/core/serialization/serialize.h>
//#include <sprockit/ptr_type.h>
#include <iostream>

namespace SST {
namespace Core {
namespace Serialization {

namespace pvt {

void
size_serializable(serializable* s, serializer& ser);

void
pack_serializable(serializable* s, serializer& ser);

void
unpack_serializable(serializable*& s, serializer& ser);

}

template <>
class serialize<serializable*> {
 
public:
void
operator()(serializable*& s, serializer& ser)
{
    switch (ser.mode()){
    case serializer::SIZER:
        pvt::size_serializable(s,ser);
        break;
    case serializer::PACK:
        pvt::pack_serializable(s,ser);
        break;
    case serializer::UNPACK:
        pvt::unpack_serializable(s,ser);
        break;  
    }
}

};


template <class T>
class serialize<T*, typename std::enable_if<std::is_base_of<SST::Core::Serialization::serializable,T>::value>::type> {
 
public:
void
operator()(T*& s, serializer& ser)
{
    serializable* sp = static_cast<serializable*>(s);
    switch (ser.mode()){
    case serializer::SIZER:
        pvt::size_serializable(sp,ser);
        break;
    case serializer::PACK:
        pvt::pack_serializable(sp,ser);
        break;
    case serializer::UNPACK:
        pvt::unpack_serializable(sp,ser);
        break;  
    }
    s = static_cast<T*>(sp);
}

};

template <class T>
void
serialize_intrusive_ptr(T*& t, serializer& ser)
{
  serializable* s = t;
  switch (ser.mode()){
case serializer::SIZER:
  pvt::size_serializable(s,ser);
  break;
case serializer::PACK:
  pvt::pack_serializable(s,ser);
  break;
case serializer::UNPACK:
  pvt::unpack_serializable(s,ser);
  t = dynamic_cast<T*>(s);
  break;  
  }  
}

template <class T>
class serialize <T, typename std::enable_if<std::is_base_of<SST::Core::Serialization::serializable,T>::value>::type> {
public:
    inline void operator()(T& t, serializer& ser){
        // T* tmp = &t;
        // serialize_intrusive_ptr(tmp, ser);
        t.serialize_order(ser);
    }
};

// Hold off on trivially_serializable for now, as it's not really safe
// in the case of inheritance
//
// template <class T>
// class serialize <T, typename std::enable_if<std::is_base_of<SST::Core::Serialization::trivially_serializable,T>::value>::type> {
// public:
//     inline void operator()(T& t, serializer& ser){
//         ser.primitive(t);
//     }
// };


} 
}
}

#endif // SERIALIZE_SERIALIZABLE_H
