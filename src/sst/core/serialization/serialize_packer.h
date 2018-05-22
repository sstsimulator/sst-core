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

#ifndef SERIALIZE_PACKER_H
#define SERIALIZE_PACKER_H

#include <sst/core/serialization/serialize_buffer_accessor.h>
#include <string>

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

class ser_packer :
  public ser_buffer_accessor
{
 public:
  template <class T>
  void
  pack(T& t){
    T* buf = ser_buffer_accessor::next<T>();
    *buf = t;
  }

  /**
   * @brief pack_buffer
   * @param buf  Must be non-null
   * @param size Must be non-zero
   */
  void
  pack_buffer(void* buf, int size);

  void
  pack_string(std::string& str);

};

} }
}
}

#endif // SERIALIZE_PACKER_H
