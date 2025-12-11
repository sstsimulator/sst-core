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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_OPTIONAL_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_OPTIONAL_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_optional.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/serializer.h"

#include <optional>
#include <string>

namespace SST::Core::Serialization {

// Serialize std::optional
template <typename T>
class serialize_impl<std::optional<T>>
{
    // ObjectMap class representing has_value state of a std::optional
    // If this ObjectMap is changed, the std::optional's has_value state is changed
    // This is only used in mapping mode
    class ObjectMapOptionalHasValue : public ObjectMapFundamental<bool>
    {
        std::optional<T>& obj;
        bool              has_value;
        ObjectMap* const  parent;

    public:
        ObjectMapOptionalHasValue(std::optional<T>& obj, ObjectMap* parent) :
            ObjectMapFundamental<bool>(&has_value),
            obj(obj),
            has_value(obj),
            parent(parent)
        {}

        void set_impl(const std::string& value) override
        {
            bool old = has_value;
            ObjectMapFundamental<bool>::set_impl(value);
            if ( has_value == old ) return;

            if ( has_value ) {
                obj.emplace();
                parent->addVariable("value", ObjectMapSerialization(*obj, "value"));
            }
            else {
                obj.reset();
                parent->removeVariable("value");
            }
        }
    };

    void operator()(std::optional<T>& obj, serializer& ser, ser_opt_t options)
    {
        bool has_value = obj.has_value();

        switch ( ser.mode() ) {
        case serializer::MAP:
            ser.mapper().map_hierarchy_start(ser.getMapName(), new ObjectMapContainer<std::optional<T>>(&obj));
            ser.mapper().map_primitive("has_value", new ObjectMapOptionalHasValue(obj, ser.mapper().get_top()));
            if ( obj ) ser.mapper().map_primitive("value", ObjectMapSerialization(*obj, "value"));
            ser.mapper().map_hierarchy_end();
            return;

        case serializer::SIZER:
            ser.size(has_value);
            break;

        case serializer::PACK:
            ser.pack(has_value);
            break;

        case serializer::UNPACK:
            ser.unpack(has_value);
            if ( has_value )
                obj.emplace();
            else
                obj.reset();
            break;
        }

        // Serialize the optional object if it is present
        if ( has_value ) SST_SER(*obj, options);
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_OPTIONAL_H
