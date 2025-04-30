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

#include "sst_config.h"

#include "sst/core/serialization/serialize.h"

namespace SST::Core::Serialization::pvt {

void
serialize_array(serializer& ser, void* data, ser_opt_t opt, size_t size,
    void serialize_array_element(serializer& ser, void* data, ser_opt_t opt, size_t index))
{
    for ( size_t index = 0; index < size; ++index )
        serialize_array_element(ser, data, opt, index);
}

void
serialize_array_map(serializer& ser, void* data, ser_opt_t opt, size_t size, ObjectMap* map,
    void serialize_array_map_element(serializer& ser, void* data, ser_opt_t opt, size_t index, const char* name))
{
    ser.mapper().map_hierarchy_start(ser.getMapName(), map);
    for ( size_t index = 0; index < size; ++index )
        serialize_array_map_element(ser, data, opt, index, std::to_string(index).c_str());
    ser.mapper().map_hierarchy_end();
}

} // namespace SST::Core::Serialization::pvt
