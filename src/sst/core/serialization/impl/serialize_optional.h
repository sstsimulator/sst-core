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

namespace SST::Core::Serialization {

// Serialize std::optional
template <typename T>
class serialize_impl<std::optional<T>>
{
    void operator()(std::optional<T>& obj, serializer& ser, ser_opt_t options)
    {
        bool has_value = false;

        switch ( ser.mode() ) {
        case serializer::SIZER:
            has_value = obj.has_value();
            ser.size(has_value);
            break;

        case serializer::PACK:
            has_value = obj.has_value();
            ser.pack(has_value);
            break;

        case serializer::UNPACK:
            ser.unpack(has_value);
            if ( has_value )
                obj.emplace();
            else
                obj.reset();
            break;

        case serializer::MAP:
        {
            // TODO: how to map std::optional ?
            return;
        }
        }

        // Serialize the optional object if it is present
        if ( has_value ) SST_SER(*obj, options);
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_OPTIONAL_H
