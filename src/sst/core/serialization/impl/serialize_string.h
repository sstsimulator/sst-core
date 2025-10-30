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

#include <string>
#include <type_traits>
#include <vector>

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

    std::string getType() override
    {
        // The demangled name for std::string is ridiculously long, so
        // just return "std::string"
        return "std::string";
    }

    explicit ObjectMapString(std::string* addr) :
        addr_(addr)
    {}

    /**
       Disallow copying and assignment
     */

    ObjectMapString(const ObjectMapString&)            = delete;
    ObjectMapString& operator=(const ObjectMapString&) = delete;

    ~ObjectMapString() override = default;
};

template <typename T>
class serialize_impl<T, std::enable_if_t<std::is_same_v<std::remove_pointer_t<T>, std::string>>>
{
    void operator()(T& str, serializer& ser, ser_opt_t options)
    {
        // sPtr is a reference to either str if it's a pointer, or to &str if it's not
        const auto& sPtr = get_ptr(str);
        const auto  mode = ser.mode();
        if ( mode == serializer::MAP ) {
            if ( options & SerOption::map_read_only ) {
                ser.mapper().setNextObjectReadOnly();
            }
            ser.mapper().map_primitive(ser.getMapName(), new ObjectMapString(sPtr));
        }
        else {
            if constexpr ( std::is_pointer_v<T> ) {
                if ( mode == serializer::UNPACK ) str = new std::string();
            }
            ser.string(*sPtr);
        }
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_STRING_H
