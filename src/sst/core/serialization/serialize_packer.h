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

  void
  pack_buffer(void* buf, int size);

  void
  pack_string(std::string& str);

};

} }
}
}

#endif // SERIALIZE_PACKER_H
