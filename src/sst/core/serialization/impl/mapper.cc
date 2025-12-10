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

#define SST_INCLUDING_SERIALIZER_H
#include "sst/core/serialization/impl/mapper.h"
#undef SST_INCLUDING_SERIALIZER_H
#include "sst/core/serialization/objectMap.h"

namespace SST::Core::Serialization::pvt {

void
ser_mapper::map_primitive(const std::string& name, ObjectMap* map)
{
    obj_.back()->addVariable(name, map);
    if ( next_item_read_only ) {
        next_item_read_only = false;
        map->setReadOnly();
    }
}

void
ser_mapper::map_container(const std::string& name, ObjectMap* map)
{
    obj_.back()->addVariable(name, map);
    if ( next_item_read_only ) {
        next_item_read_only = false;
    }
}

void
ser_mapper::map_existing_object(const std::string& name, ObjectMap* map)
{
    map->incRefCount();
    obj_.back()->addVariable(name, map);
    if ( next_item_read_only ) {
        next_item_read_only = false;
    }
}

void
ser_mapper::map_hierarchy_start(const std::string& name, ObjectMap* map)
{
    obj_.back()->addVariable(name, map);
    obj_.push_back(map);

    indent++;
    if ( next_item_read_only ) {
        next_item_read_only = false;
    }
}

void
ser_mapper::map_hierarchy_end()
{
    obj_.pop_back();
    indent--;
}

void
ser_mapper::report_object_map(ObjectMap* ptr)
{
    pointer_map.insert_or_assign(reinterpret_cast<uintptr_t>(ptr->getAddr()), reinterpret_cast<uintptr_t>(ptr));
}

ObjectMap*
ser_mapper::check_pointer_map(uintptr_t ptr)
{
    auto it = pointer_map.find(ptr);
    return it != pointer_map.end() ? reinterpret_cast<ObjectMap*>(it->second) : nullptr;
}

} // namespace SST::Core::Serialization::pvt
