// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <sst/core/serialization/serializer.h>
//#include <sprockit/debug.h>
//DeclareDebugSlot(serialize);

#include <iostream>
#include <typeinfo>

namespace SST {
namespace Core {
namespace Serialization {

template <class T, class Enable = void>
class serialize {
 public:
  inline void operator()(T& t, serializer& ser){
      // If the default gets called, then it's actually invalid
      // because we don't know how to serialize it.

      // This is a bit strange, but if I just do a
      // static_assert(false) it always triggers, but if I use
      // std::is_* then it seems to only trigger if something expands
      // to this version of the template.
      // static_assert(false,"Trying to serialize an object that is not serializable.");
      static_assert(std::is_fundamental<T>::value,"Trying to serialize an object that is not serializable.");
      static_assert(!std::is_fundamental<T>::value,"Trying to serialize an object that is not serializable.");
  }
};

template <class T>
class serialize <T, typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type> {
 public:
  inline void operator()(T& t, serializer& ser){
      ser.primitive(t); 
  }
};

template <>
class serialize<bool> {
 public:
  void operator()(bool &t, serializer& ser){
    int bval = t;
    ser.primitive(bval);
    t = bool(bval);
  }
};


template <class T>
inline void
operator&(serializer& ser, T& t){
    serialize<T>()(t, ser);
}

}
}
}

#include <sst/core/serialization/serialize_array.h>
#include <sst/core/serialization/serialize_list.h>
#include <sst/core/serialization/serialize_map.h>
#include <sst/core/serialization/serialize_set.h>
#include <sst/core/serialization/serialize_vector.h>
#include <sst/core/serialization/serialize_string.h>

#endif // SERIALIZE_H
