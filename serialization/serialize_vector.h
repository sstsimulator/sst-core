#ifndef SERIALIZE_VECTOR_H
#define SERIALIZE_VECTOR_H

#include <vector>
#include <sst/core/serialization/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {

template <class T>
class serialize<std::vector<T> > {
  typedef std::vector<T> Vector; 
 public:
  void
  operator()(Vector& v, serializer& ser) {
    switch(ser.mode())
    {
    case serializer::SIZER: {
      size_t size = v.size();
      ser.size(size);
      break;
    }
    case serializer::PACK: {
      size_t size = v.size();
      ser.pack(size);
      break;
    }
    case serializer::UNPACK: {
      size_t s;
      ser.unpack(s);
      v.resize(s);
      break;
    }
    }
  
    for (int i=0; i < v.size(); ++i){
      serialize<T>()(v[i], ser);
    }
  }
  
};

}
}
}

#endif // SERIALIZE_VECTOR_H
