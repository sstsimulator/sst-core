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

#include "sst/core/serialization/serializable.h"

#include <iostream>

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

static const long null_ptr_id = -1;

void
size_serializable(serializable_base* s, serializer& ser)
{
    long dummy = 0;
    ser.size(dummy);
    if ( s ) { s->serialize_order(ser); }
}

void
pack_serializable(serializable_base* s, serializer& ser)
{
    if ( s ) {
        long cls_id = s->cls_id();
        ser.pack(cls_id);
        s->serialize_order(ser);
    }
    else {
        long id = null_ptr_id;
        ser.pack(id);
    }
}

void
unpack_serializable(serializable_base*& s, serializer& ser)
{
    long cls_id;
    ser.unpack(cls_id);
    if ( cls_id == null_ptr_id ) { s = nullptr; }
    else {
        s = SST::Core::Serialization::serializable_factory::get_serializable(cls_id);
        ser.report_new_pointer(reinterpret_cast<uintptr_t>(s));
        s->serialize_order(ser);
    }
}

void
map_serializable(serializable_base*& s, serializer& ser, const char* name)
{
    if ( s ) {
        SST::Core::Serialization::ObjectMap* obj_map = new SST::Core::Serialization::ObjectMapClass(s, s->cls_name());
        ser.report_object_map(obj_map);
        ser.mapper().map_hierarchy_start(name, obj_map);
        s->serialize_order(ser);
        ser.mapper().map_hierarchy_end();
    }
}

} // namespace pvt
} // namespace Serialization
} // namespace Core
} // namespace SST
