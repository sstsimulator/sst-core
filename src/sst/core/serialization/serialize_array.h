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

#ifndef SERIALIZE_ARRAY_H
#define SERIALIZE_ARRAY_H

#include <sst/core/serialization/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

template <class TPtr, class IntType>
class ser_array_wrapper
{
 public:
  TPtr& bufptr;
  IntType& sizeptr;
  ser_array_wrapper(TPtr& buf, IntType& size) :
    bufptr(buf), sizeptr(size) {}

};

template <class IntType>
class ser_buffer_wrapper
{
 public:
  void*& bufptr;
  IntType& sizeptr;
  ser_buffer_wrapper(void*& buf, IntType& size) :
    bufptr(buf), sizeptr(size) {}
};

}

template <class T, int N>
class serialize<T[N]> {
 public:
  void operator()(T arr[N], serializer& ser){
    ser.array<T,N>(arr);
  }
};

/** I have typedefing pointers, but no other way.
 *  T could be "void and TPtr void* */
template <class TPtr, class IntType>
pvt::ser_array_wrapper<TPtr,IntType>
array(TPtr& buf, IntType& size)
{
  return pvt::ser_array_wrapper<TPtr,IntType>(buf, size);
}

template <class IntType>
pvt::ser_buffer_wrapper<IntType>
buffer(void*& buf, IntType& size)
{
  return pvt::ser_buffer_wrapper<IntType>(buf,size);
}

template <class TPtr, class IntType>
inline void
operator&(serializer& ser, pvt::ser_array_wrapper<TPtr,IntType> arr){
  ser.binary(arr.bufptr, arr.sizeptr);
}

template <class IntType>
inline void
operator&(serializer& ser, pvt::ser_buffer_wrapper<IntType> buf){
  char* tmp = (char*) buf.bufptr;
  ser.binary(tmp, buf.sizeptr);
  buf.bufptr = tmp;
}

}
}
}
#endif // SERIALIZE_ARRAY_H
