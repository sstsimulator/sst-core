#ifndef SERIALIZE_LIST_H
#define SERIALIZE_LIST_H

#include <list>
#include <sst/core/serialization/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {

template <class T>
class serialize <std::list<T> > {
 typedef std::list<T> List; 
public:
 void
 operator()(List& v, serializer& ser) {
  typedef typename List::iterator iterator;
  switch(ser.mode())
  {
  case serializer::SIZER: {
    size_t size = v.size();
    ser.size(size);
    iterator it, end = v.end();
    for (it=v.begin(); it != end; ++it){
      T& t = *it;
      serialize<T>()(t, ser);
    }
    break;
  }
  case serializer::PACK: {
    size_t size = v.size();
    ser.pack(size);
    iterator it, end = v.end();
    for (it=v.begin(); it != end; ++it){
      T& t = *it;
      serialize<T>()(t, ser);
    }
    break;
  }
  case serializer::UNPACK: {
    size_t size;
    ser.unpack(size);
    for (int i=0; i < size; ++i){
      T t;
      serialize<T>()(t, ser);
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
#endif // SERIALIZE_LIST_H
