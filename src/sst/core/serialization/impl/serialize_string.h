// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_STRING_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_STRING_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_string.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

namespace SST::Core::Serialization {

class ObjectMapString : public ObjectMap
{
protected:
    std::string* addr_;

public:
    /**
       Get the address of the represented object

       @return address of represented object
     */
    void* getAddr() override { return addr_; }

    std::string get() override { return *addr_; }

    void set_impl(const std::string& value) override { *addr_ = value; }

    virtual bool isFundamental() override { return true; }

    /**
       Get the list of child variables contained in this ObjectMap,
       which in this case will be empty.

       @return Refernce to vector containing ObjectMaps for this
       ObjectMap's child variables. This vector will be empty because
       strings have no children
     */
    const std::vector<std::pair<std::string, ObjectMap*>>& getVariables() override { return emptyVars; }

    std::string getType() override
    {
        // The demangled name for std::string is ridiculously long, so
        // just return "std::string"
        return "std::string";
    }

    ObjectMapString(std::string* addr) : ObjectMap(), addr_(addr) {}
};

template <>
class serialize_impl<std::string>
{
public:
    void operator()(std::string& str, serializer& ser) { ser.string(str); }
    void operator()(std::string& str, serializer& ser, const char* name)
    {
        ObjectMapString* obj_map = new ObjectMapString(&str);
        ser.mapper().map_primitive(name, obj_map);
    }
};

template <>
class serialize_impl<std::string*>
{
public:
    void operator()(std::string*& str, serializer& ser)
    {
        // Nullptr is checked for in serialize<T>(), so just serialize
        // as if it was non-pointer
        if ( ser.mode() == serializer::UNPACK ) { str = new std::string(); }
        ser.string(*str);
    }
    void operator()(std::string*& str, serializer& ser, const char* name)
    {
        ObjectMapString* obj_map = new ObjectMapString(str);
        ser.mapper().map_primitive(name, obj_map);
    }
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_STRING_H
