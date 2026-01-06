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

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

namespace SST::Core::Serialization::pvt {

class ser_mapper
{
    std::unordered_map<uintptr_t, uintptr_t> pointer_map_;
    std::deque<ObjectMap*>                   obj_;

public:
    explicit ser_mapper(ObjectMap* object) :
        obj_ { object }
    {}

    ObjectMap* get_top() const { return obj_.back(); }

    void map_object(const std::string& name, ObjectMap* map) { obj_.back()->addVariable(name, map); }

    void map_existing_object(const std::string& name, ObjectMap* map)
    {
        map->incRefCount();
        obj_.back()->addVariable(name, map);
    }

    void map_hierarchy_start(const std::string& name, ObjectMap* map)
    {
        obj_.back()->addVariable(name, map);
        obj_.push_back(map);
    }

    void map_hierarchy_end() { obj_.pop_back(); }

    void report_object_map(ObjectMap* ptr)
    {
        pointer_map_.insert_or_assign(reinterpret_cast<uintptr_t>(ptr->getAddr()), reinterpret_cast<uintptr_t>(ptr));
    }

    ObjectMap* check_pointer_map(uintptr_t ptr)
    {
        auto it = pointer_map_.find(ptr);
        return it != pointer_map_.end() ? reinterpret_cast<ObjectMap*>(it->second) : nullptr;
    }

}; // class ser_wrapper

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_MAPPER_H
