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

#ifndef SERIALIZE_DEQUE_H
#define SERIALIZE_DEQUE_H

#include <deque>
#include <sst/core/serialization/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {

template <class T>
class serialize<std::deque<T> > {
  typedef std::deque<T> Deque;
 
public:
  void
  operator()(Deque& v, serializer& ser) {
      switch(ser.mode()) {
      case serializer::SIZER: {
          size_t size = v.size();
          ser.size(size);
          for (auto it = v.begin(); it != v.end(); ++it){
              T& t = const_cast<T&>(*it); 
              serialize<T>()(t,ser);
          }
          break;
      }
      case serializer::PACK: {
          size_t size = v.size();
          ser.pack(size);
          for (auto it = v.begin(); it != v.end(); ++it){
              T& t = const_cast<T&>(*it); 
              serialize<T>()(t,ser);
          }
          break;
      }
      case serializer::UNPACK: {
          size_t size;
          ser.unpack(size);
          for (int i=0; i < size; ++i){
              T t;
              serialize<T>()(t,ser);
              v.push_back(t);
          }
          break;
      }
      }
  }
};

}
}
}
#endif // SERIALIZE_DEQUE_H
