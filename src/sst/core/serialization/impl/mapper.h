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

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace SST::Core::Serialization {

class ObjectMap;

namespace pvt {

class ser_mapper
{
    std::map<uintptr_t, uintptr_t> pointer_map;
    std::vector<ObjectMap*>        obj_;
    bool                           next_item_read_only = false;
    int                            indent              = 0;

public:
    ser_mapper() = default;

    explicit ser_mapper(ObjectMap* object) :
        obj_ { object }
    {}

    void       map_primitive(const std::string& name, ObjectMap* map);
    void       map_container(const std::string& name, ObjectMap* map);
    void       map_existing_object(const std::string& name, ObjectMap* map);
    void       map_hierarchy_start(const std::string& name, ObjectMap* map);
    void       map_hierarchy_end();
    void       setNextObjectReadOnly() { next_item_read_only = true; }
    void       report_object_map(ObjectMap* ptr);
    ObjectMap* check_pointer_map(uintptr_t ptr);
};

} // namespace pvt
} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_IMPL_MAPPER_H
