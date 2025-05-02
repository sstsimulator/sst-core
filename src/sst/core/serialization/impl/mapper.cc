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

#include <iostream>

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
ser_mapper::init(ObjectMap* object)
{
    obj_.push_back(object);
}

void
ser_mapper::reset()
{
    obj_.clear();
}

void
ser_mapper::setNextObjectReadOnly()
{
    next_item_read_only = true;
}

} // namespace SST::Core::Serialization::pvt
