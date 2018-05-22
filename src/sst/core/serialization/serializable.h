/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 *  Copyright (c) 2009-2018 NTESS.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-NA0003525 with NTESS,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top
 *  SST/macroscale directory.
 */

#ifndef SST_CORE_SERIALIZATION_SERIALIZABLE_H
#define SST_CORE_SERIALIZATION_SERIALIZABLE_H

#include <sst/core/serialization/serializer.h>
#include <sst/core/warnmacros.h>
#include <unordered_map>
#include <typeinfo>
#include <stdint.h>

namespace SST {
namespace Core {
namespace Serialization {

namespace pvt {

// Functions to implement the hash for cls_id at compile time.  The
// hash function being implemented is:

inline uint32_t type_hash(const char* key)
{
  int len = ::strlen(key);
  uint32_t hash = 0;
  for(int i = 0; i < len; ++i)
  {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash;
}

// Using constexpr is very limited, so the implementation is a bit
// convoluted.  May be able to streamline later.

// computes hash ^= (hash >> 6)
constexpr uint32_t B(const uint32_t b)
{
    return b ^ (b >> 6);
}

// computes hash += (hash << 10)
constexpr uint32_t A(const uint32_t a)
{
    return B( (a << 10 ) + a);
}

// recursive piece that computes the for loop
template<size_t idx>
constexpr uint32_t ct_hash_rec(const char* str)
{
    return A( str[idx] + ct_hash_rec<idx-1>(str) );
}

// End of the recursion (i.e. when you've walked back off the front of
// the string
template<>
constexpr uint32_t ct_hash_rec<size_t(-1)>(const char* UNUSED(str))
{
    return 0;
}

// computes hash += (hash << 15)
constexpr uint32_t E(const uint32_t e)
{
    return (e << 15) + e;
}

// computes hash ^= (hash >> 11)
constexpr uint32_t D(const uint32_t d)
{
    return E( (d >> 11) ^ d);
}

// computes hash += (hash << 3)
constexpr uint32_t C(const uint32_t c)
{
    return D( (c << 3 ) + c);
}

// Main function that computes the final manipulations after calling
// the recursive function to compute the for loop.
template<size_t idx>
constexpr uint32_t ct_hash(const char* str)
{
    return C(ct_hash_rec<idx>(str));
}

// Macro that should be used to call the compile time hash function
#define COMPILE_TIME_HASH(x) (::SST::Core::Serialization::pvt::ct_hash<sizeof(x)-2>(x))


} // namespace pvt

class serializable
{
public:
    virtual const char*
    cls_name() const = 0;

    virtual void
    serialize_order(serializer& ser) = 0;
    
    virtual uint32_t
    cls_id() const = 0;
    virtual std::string serialization_name() const = 0;
    
    virtual ~serializable() { }
    
protected:
    typedef enum { ConstructorFlag } cxn_flag_t;
    static void serializable_abort(uint32_t line, const char* file, const char* func, const char* obj);
};

template <class T>
class serializable_type
{
};

#define ImplementVirtualSerializable(obj)     \
   protected:                                \
       obj(cxn_flag_t flag){}


#define NotSerializable(obj) \
 public:  \
  static void \
  throw_exc(){ \
     serializable_abort(CALL_INFO_LONG, #obj); \
  } \
  virtual void \
  serialize_order(SST::Core::Serialization::serializer& UNUSED(sst)) override {    \
    throw_exc(); \
  } \
  virtual uint32_t \
  cls_id() const override { \
    throw_exc(); \
    return -1; \
  } \
  static obj* \
  construct_deserialize_stub() { \
    throw_exc(); \
    return 0; \
  } \
  virtual std::string \
  serialization_name() const override { \
    throw_exc(); \
    return ""; \
  } \
  virtual const char* \
  cls_name() const override { \
    throw_exc(); \
    return ""; \
  }

#define ImplementSerializableDefaultConstructor(obj)    \
 public: \
  virtual const char* \
  cls_name() const override { \
    return #obj; \
  } \
  virtual uint32_t \
  cls_id() const override { \
    return SST::Core::Serialization::serializable_builder_impl<obj>::static_cls_id(); \
  }           \
  static obj* \
  construct_deserialize_stub() { \
    return new obj; \
  } \
  virtual std::string \
  serialization_name() const override { \
    return #obj; \
  } \
private:\
  friend class SST::Core::Serialization::serializable_builder_impl<obj>;  \
  static bool                                                 \
  you_forgot_to_add_ImplementSerializable_to_this_class() { \
    return false; \
  }

#define ImplementSerializable(obj) \
 public: \
 ImplementSerializableDefaultConstructor(obj)


class serializable_builder
{
 public:
  virtual serializable*
  build() const = 0;

  virtual ~serializable_builder(){}

  virtual const char*
  name() const = 0;

  virtual uint32_t
  cls_id() const = 0;

  virtual bool
  sanity(serializable* ser) = 0;
};

template<class T>
class serializable_builder_impl : public serializable_builder
{
 protected:
    static const char* name_;
    static const uint32_t cls_id_;

 public:
  serializable*
  build() const override {
      return T::construct_deserialize_stub();
  }

  const char*
  name() const override {
    return name_;
  }

  uint32_t
  cls_id() const override {
      return cls_id_;
  }

  static uint32_t
  static_cls_id() {
    return cls_id_;
  }

  static const char*
  static_name() {
      return name_;
  }
    
  bool
  sanity(serializable* ser) override {
    return (typeid(T) == typeid(*ser));
  }
};

class serializable_factory
{
protected:
  typedef std::unordered_map<long, serializable_builder*> builder_map;
  static builder_map* builders_;

 public:
  static serializable*
  get_serializable(uint32_t cls_id);

  /**
      @return The cls id for the given builder
  */
  static uint32_t
  // add_builder(serializable_builder* builder, uint32_t cls_id);
  add_builder(serializable_builder* builder, const char* name);

  static bool
  sanity(serializable* ser, uint32_t cls_id) {
    return (*builders_)[cls_id]->sanity(ser);
  }

  static void
  delete_statics();

};

template<class T> const char* serializable_builder_impl<T>::name_ = typeid(T).name();
template<class T> const uint32_t serializable_builder_impl<T>::cls_id_
  = serializable_factory::add_builder(new serializable_builder_impl<T>,
       typeid(T).name());

// Hold off on trivially_serializable for now, as it's not really safe
// in the case of inheritance
//
// class trivially_serializable {
// };

}  // namespace Serialization
}  // namespace Core
}  // namespace SST

#define SerializableName(obj) #obj

#define DeclareSerializable(obj)


#include <sst/core/serialization/serialize_serializable.h>

#endif

