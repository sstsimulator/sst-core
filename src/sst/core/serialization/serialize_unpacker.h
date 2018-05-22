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

#ifndef SERIALIZE_UNPACKER_H
#define SERIALIZE_UNPACKER_H

#include <sst/core/serialization/serialize_buffer_accessor.h>

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

class ser_unpacker :
  public ser_buffer_accessor
{
 public:
  template <class T>
  void
  unpack(T& t){
    T* bufptr = ser_buffer_accessor::next<T>();
    t = *bufptr;
  }

  /**
   * @brief unpack_buffer
   * @param buf   Must unpack to non-null buffer
   * @param size  Must be non-zero
   */
  void
  unpack_buffer(void* buf, int size);

  void
  unpack_string(std::string& str);

};

} }
}
}

#endif // SERIALIZE_UNPACKER_H
