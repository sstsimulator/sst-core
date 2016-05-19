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

template <class T>
class serialize {
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
