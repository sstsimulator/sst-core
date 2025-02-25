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

#ifndef SST_CORE_SERIALIZATION_IMPL_MAPPER_H
#define SST_CORE_SERIALIZATION_IMPL_MAPPER_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/mapper.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include "sst/core/serialization/objectMap.h"

#include <string>
#include <typeinfo>
#include <vector>

namespace SST::Core::Serialization::pvt {

class ser_mapper
{
    std::vector<ObjectMap*> obj_;
    bool                    next_item_read_only = false;

public:
    void map_primitive(const std::string& name, ObjectMap* map)
    {
        obj_.back()->addVariable(name, map);
        if ( next_item_read_only ) {
            next_item_read_only = false;
            map->setReadOnly();
        }
    }

    void map_container(const std::string& name, ObjectMap* map)
    {
        obj_.back()->addVariable(name, map);
        if ( next_item_read_only ) { next_item_read_only = false; }
    }

    void map_existing_object(const std::string& name, ObjectMap* map)
    {
        map->incRefCount();
        obj_.back()->addVariable(name, map);
        if ( next_item_read_only ) { next_item_read_only = false; }
    }

    void map_hierarchy_start(const std::string& name, ObjectMap* map)
    {
        obj_.back()->addVariable(name, map);
        obj_.push_back(map);

        indent++;
        if ( next_item_read_only ) { next_item_read_only = false; }
    }

    void map_hierarchy_end()
    {
        obj_.pop_back();
        indent--;
    }

    void init(ObjectMap* object) { obj_.push_back(object); }

    void reset() { obj_.clear(); }

    /**
     * @brief pack_buffer
     * @param buf  Must be non-null
     * @param size Must be non-zero
     */
    void map_buffer(void* buf, int size);

    void setNextObjectReadOnly() { next_item_read_only = true; }

private:
    int indent = 0;
};

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_MAPPER_H
