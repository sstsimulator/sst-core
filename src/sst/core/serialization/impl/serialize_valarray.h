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

#ifndef SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VALARRAY_H
#define SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VALARRAY_H

#ifndef SST_INCLUDING_SERIALIZE_H
#warning \
    "The header file sst/core/serialization/impl/serialize_valarray.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serialize.h"
#endif

#include "sst/core/serialization/impl/serialize_utility.h"
#include "sst/core/serialization/serializer.h"

#include <valarray>

namespace SST::Core::Serialization {

// Serialize valarray
template <typename T>
class serialize_impl<T, std::enable_if_t<is_same_type_template_v<std::remove_pointer_t<T>, std::valarray>>>
{
    void operator()(T& obj, serializer& ser, ser_opt_t UNUSED(options))
    {
        const auto& objPtr = get_ptr(obj);
        size_t      size   = 0;
        switch ( ser.mode() ) {
        case serializer::SIZER:
            size = objPtr->size();
            ser.size(size);
            break;

        case serializer::PACK:
            size = objPtr->size();
            ser.pack(size);
            break;

        case serializer::UNPACK:
            ser.unpack(size);
            if constexpr ( std::is_pointer_v<T> )
                obj = new std::remove_pointer_t<T>(size);
            else
                obj.resize(size);
            break;

        case serializer::MAP:
            size = objPtr->size();
            ser.mapper().map_hierarchy_start(
                ser.getMapName(), new ObjectMapContainer<std::remove_pointer_t<T>>(objPtr));
            for ( size_t i = 0; i < size; ++i )
                sst_ser_object(ser, (*objPtr)[i], SerOption::none, std::to_string(i).c_str());
            ser.mapper().map_hierarchy_end();
            return;
        }
        for ( size_t i = 0; i < size; ++i )
            SST_SER((*objPtr)[i]);
    }

    SST_FRIEND_SERIALIZE();
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_SERIALIZE_VALARRAY_H
