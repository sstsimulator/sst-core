#ifndef SERIALIZE_SERIALIZABLE_H
#define SERIALIZE_SERIALIZABLE_H

#include <sst/core/serialization/serializable.h>
#include <sst/core/serialization/serializer.h>
//#include <sprockit/ptr_type.h>
#include <iostream>

namespace SST {
namespace Core {
namespace Serialization {

namespace pvt {

void
size_serializable(serializable* s, serializer& ser);

void
pack_serializable(serializable* s, serializer& ser);

void
unpack_serializable(serializable*& s, serializer& ser);

}

template <>
class serialize<serializable*> {
 
public:
void
operator()(serializable*& s, serializer& ser)
{
    switch (ser.mode()){
    case serializer::SIZER:
        pvt::size_serializable(s,ser);
        break;
    case serializer::PACK:
        pvt::pack_serializable(s,ser);
        break;
    case serializer::UNPACK:
        pvt::unpack_serializable(s,ser);
        break;  
    }
}

};


template <class T>
class serialize<T*> {
 
public:
void
operator()(T*& s, serializer& ser)
{
    serializable* sp = static_cast<serializable*>(s);
    switch (ser.mode()){
    case serializer::SIZER:
        pvt::size_serializable(sp,ser);
        break;
    case serializer::PACK:
        pvt::pack_serializable(sp,ser);
        break;
    case serializer::UNPACK:
        pvt::unpack_serializable(sp,ser);
        break;  
    }
    s = static_cast<T*>(sp);
}

};

template <class T>
void
serialize_intrusive_ptr(T*& t, serializer& ser)
{
  serializable* s = t;
  switch (ser.mode()){
case serializer::SIZER:
  pvt::size_serializable(s,ser);
  break;
case serializer::PACK:
  pvt::pack_serializable(s,ser);
  break;
case serializer::UNPACK:
  pvt::unpack_serializable(s,ser);
  t = dynamic_cast<T*>(s);
  break;  
  }  
}

#if 0
template <class T>
class serialize<sprockit::refcount_ptr<T> > {
 public:
  void operator()(sprockit::refcount_ptr<T>& o, serializer& ser){
    T* tmp = o.get();
    serialize_intrusive_ptr(tmp, ser);
    o = tmp;
  }
};

template <class T>
class serialize<const sprockit::refcount_ptr<T> > {
 public:
  void operator()(const sprockit::refcount_ptr<T>& o, serializer& ser){
    T* tmp = o.get();
    serialize_intrusive_ptr(tmp, ser);
  }
};


template <class T>
class serialize<sprockit::refcount_ptr<const T> > {
 public:
  void operator()(sprockit::refcount_ptr<const T>& o, serializer& ser){
    T* tmp = const_cast<T*>(o.get());
    serialize_intrusive_ptr(tmp, ser);
    o = tmp;    
  }
};

#else

template <>
class serialize<serializable> {
 public:
  void operator()(serializable& o, serializer& ser){
    serializable* tmp = &o;
    serialize_intrusive_ptr(tmp, ser);
  }
};

// template <class T>
// class serialize<const sprockit::refcount_ptr<T> > {
//  public:
//   void operator()(const T& o, serializer& ser){
//     T* tmp = &o;
//     serialize_intrusive_ptr(tmp, ser);
//   }
// };

#endif

} 
}
}

#endif // SERIALIZE_SERIALIZABLE_H
