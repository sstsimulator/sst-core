/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 *  Copyright (c) 2009 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top
 *  SST/macroscale directory.
 */

#ifndef SST_CORE_SERIALIZATION_SERIALIZABLE_H
#define SST_CORE_SERIALIZATION_SERIALIZABLE_H

#include <sst/core/serialization/serialize.h>
#include <sst/core/output.h>
//#include <sprockit/unordered.h>
#include <unordered_map>
#include <typeinfo>
#include <stdint.h>

namespace SST {
namespace Core {
namespace Serialization {

class serializable
{
 public:
  virtual const char*
  cls_name() const = 0;

  virtual void
  serialize_order(serializer& ser) = 0;

  virtual uint32_t
  cls_id() const = 0;

  virtual ~serializable() { }

 protected:
  typedef enum { ConstructorFlag } cxn_flag_t;
};

template <class T>
class serializable_type
{
 protected:
  static uint32_t cls_id_;

  virtual T*
  you_forgot_to_add_ImplementSerializable_to_this_class() = 0;

 public:
  virtual ~serializable_type() {}

  uint32_t
  cls_id() const {
    return cls_id_;
  }

};

//#define ImplementVirtualSerializable(obj)     \
//    protected:                                \
//        obj(cxn_flag_t flag){}


#define NotSerializable(obj) \
 public:  \
  static void \
  throw_exc(){ \
     Output ser_abort("", 5, -1, Output::STDERR); \
     ser_abort.fatal(CALL_INFO_LONG, -1, "ERROR: type %s should not be serialized\n",#obj); \
  } \
  virtual void \
  serialize_order(SST::Core::Serialization::serializer& sst){    \
    throw_exc(); \
  } \
  virtual uint32_t \
  cls_id() const { \
    throw_exc(); \
    return -1; \
  } \
  static obj* \
  construct_deserialize_stub() { \
    throw_exc(); \
    return 0; \
  } \
  virtual std::string \
  serialization_name() const { \
    throw_exc(); \
    return ""; \
  } \
  virtual const char* \
  cls_name() const { \
    throw_exc(); \
    return ""; \
  } \
  virtual obj* \
  you_forgot_to_add_ImplementSerializable_to_this_class() { \
    return 0; \
  }

#define ImplementSerializableDefaultConstructor(obj)    \
 public: \
  virtual const char* \
  cls_name() const { \
    return #obj; \
  } \
  virtual uint32_t \
  cls_id() const { \
      return ::SST::Core::Serialization::serializable_type< obj >::cls_id(); \
  } \
  static obj* \
  construct_deserialize_stub() { \
    return new obj; \
  } \
  virtual std::string \
  serialization_name() const { \
    return #obj; \
  } \
  virtual obj* \
  you_forgot_to_add_ImplementSerializable_to_this_class() { \
    return 0; \
  } \
  // static obj* \
  // construct_deserialize_stub() { \
  //   return new obj; \
  // } \

#define ImplementSerializable(obj) \
 public: \
 ImplementSerializableDefaultConstructor(obj) \
 //obj(){}


class serializable_builder
{
 public:
  virtual serializable*
  build() const = 0;

  virtual ~serializable_builder(){}

  virtual const char*
  name() const = 0;

  virtual bool
  sanity(serializable* ser) = 0;
};

template<class T>
class serializable_builder_impl : public serializable_builder
{
 protected:
  static const char* name_;

 public:
  serializable*
  build() const {
      return T::construct_deserialize_stub();
  }

  const char*
  name() const {
    return name_;
  }

  bool
  sanity(serializable* ser) {
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
  add_builder(serializable_builder* builder);

  static bool
  sanity(serializable* ser, uint32_t cls_id) {
    return (*builders_)[cls_id]->sanity(ser);
  }

  static void
  delete_statics();

};

}  // namespace Serialization
}  // namespace Core
}  // namespace SST

#define SerializableName(obj) #obj

#define DeclareSerializable(...) \
namespace SST { \
namespace Core { \
namespace Serialization { \
template<> const char* serializable_builder_impl<__VA_ARGS__ >::name_ = SerializableName((__VA_ARGS__)); \
template<> uint32_t SST::Core::Serialization::serializable_type<__VA_ARGS__ >::cls_id_ = serializable_factory::add_builder(new serializable_builder_impl<__VA_ARGS__ >); \
} \
} \
}

#include <sst/core/serialization/serialize_serializable.h>

#endif

