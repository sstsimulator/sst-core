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

#ifndef SERIALIZE_SIZER_H
#define SERIALIZE_SIZER_H

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

class ser_sizer
{
 public:
  ser_sizer() :
    size_(0)
  {
  }

  template <class T>
  void
  size(T& t){
    size_ += sizeof(T);
  }

  void
  size_string(std::string& str);

  void
  add(size_t s) {
    size_ += s;
  }

  size_t
  size() const {
    return size_;
  }

  void
  reset() {
    size_ = 0;
  }

 protected:
  size_t size_;

};

} }
}
}

#endif // SERIALIZE_SIZER_H
