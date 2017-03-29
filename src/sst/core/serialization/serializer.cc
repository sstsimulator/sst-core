/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 *  Copyright (c) 2009-2017 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top
 *  SST/macroscale directory.
 */

#include <sst/core/serialization/serializer.h>
#include <sst/core/serialization/serializable.h>


namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

void
ser_unpacker::unpack_buffer(void* buf, int size)
{
  //void* only for convenience... actually a void**
  void** bufptr = (void**) buf;
  if (size == 0){
    buf = 0;
  } else {
    *bufptr = new char[size];
    char* charstr = next_str(size);
    ::memcpy(*bufptr, charstr, size);
  }
}

void
ser_packer::pack_buffer(void* buf, int size)
{
  if (buf && size != 0){
    char* charstr = next_str(size);
    ::memcpy(charstr, buf, size);
  } else {
    int nullsize = 0;
    pack(nullsize);
  }
}

void
ser_unpacker::unpack_string(std::string& str)
{
  int size;
  unpack(size);
  char* charstr = next_str(size);
  str.resize(size);
  str.assign(charstr, size);
}

void
ser_packer::pack_string(std::string& str)
{
  int size = str.size();
  pack(size);
  char* charstr = next_str(size);
  ::memcpy(charstr, str.data(), size);
}

void
ser_sizer::size_string(std::string &str)
{
  size_ += sizeof(int);
  size_ += str.size();
}

} //end ns pvt

void
serializer::string(std::string& str)
{
  switch(mode_)
  {
  case SIZER: {
    sizer_.size_string(str);
    break;
  }
  case PACK: {
    packer_.pack_string(str);
    break;
  }
  case UNPACK: {
    unpacker_.unpack_string(str);
    break;
  }
  }
}

} // end of namespace sprockit
}
}

