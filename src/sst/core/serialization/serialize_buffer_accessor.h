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

#ifndef SERIALIZE_ACCESSOR_H
#define SERIALIZE_ACCESSOR_H

#include <sst/core/warnmacros.h>
#include <cstring>
#include <exception>
//#include <sst/core/serialization/errors.h>

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

// class ser_buffer_overrun : public spkt_error {
class ser_buffer_overrun : public std::exception {
 public:
    ser_buffer_overrun(int UNUSED(maxsize))
    // ser_buffer_overrun(int maxsize) :
      //spkt_error(sprockit::printf("serialization overrun buffer of size %d", maxsize))
  {
  }
};

class ser_buffer_accessor {
 public:
  template <class T>
  T*
  next(){
    T* ser_buffer = reinterpret_cast<T*>(bufptr_);
    bufptr_ += sizeof(T);
    size_ += sizeof(T);
    if (size_ > max_size_) throw ser_buffer_overrun(max_size_);
    return ser_buffer;
  }

  char*
  next_str(size_t size){
    char* ser_buffer = reinterpret_cast<char*>(bufptr_);
    bufptr_ += size;
    size_ += size;
    if (size_ > max_size_) throw ser_buffer_overrun(max_size_);
    return ser_buffer;
  }

  size_t
  size() const {
    return size_;
  }

  size_t
  max_size() const {
    return max_size_;
  }

  void
  init(void* buffer, size_t size){
    bufstart_ = reinterpret_cast<char*>(buffer);
    max_size_ = size;
    reset();
  }

  void
  clear(){
    bufstart_ = bufptr_ = 0;
    max_size_ = size_ = 0;
  }

  void
  reset(){
    bufptr_ = bufstart_;
    size_ = 0;
  }

 protected:
  ser_buffer_accessor() :
    bufstart_(0),
    bufptr_(0),
    size_(0)
  {
  }

 protected:
  char* bufstart_;
  char* bufptr_;
  size_t size_;
  size_t max_size_;

};

} }
}
}

#endif // SERIALIZE_ACCESSOR_H
