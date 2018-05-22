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

#ifndef SERIALIZE_MAP_H
#define SERIALIZE_MAP_H

#include <map>
#include <sst/core/serialization/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {

template <class Key, class Value>
class serialize<std::map<Key,Value> > {
  typedef std::map<Key,Value> Map;
 public:
  void operator()(Map& m, serializer& ser){
    typedef typename std::map<Key,Value>::iterator iterator;
    switch(ser.mode())
    {
    case serializer::SIZER: {
      size_t size = m.size();
      ser.size(size);
      iterator it, end = m.end();
      for (it=m.begin(); it != end; ++it){
        //keys are const values - annoyingly
        serialize<Key>()(const_cast<Key&>(it->first), ser);
        serialize<Value>()(it->second, ser);
      }
      break;
    }
    case serializer::PACK: {
      size_t size = m.size();
      ser.pack(size);
      iterator it, end = m.end();
      for (it=m.begin(); it != end; ++it){
        serialize<Key>()(const_cast<Key&>(it->first), ser);
        serialize<Value>()(it->second, ser);
      }
      break;
    }
    case serializer::UNPACK: {
      size_t size;
      ser.unpack(size);
      for (size_t i=0; i < size; ++i){
        Key k = {};
        Value v = {};
        serialize<Key>()(k, ser);
        serialize<Value>()(v, ser);
        m[k] = v;
      }
      break;
    }
    }    
  }
};

}
}
}

#endif // SERIALIZE_MAP_H
