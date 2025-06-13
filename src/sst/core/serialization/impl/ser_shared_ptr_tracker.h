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

#ifndef SST_CORE_SERIALIZATION_IMPL_SER_SHARED_PTR_TRACKER_H
#define SST_CORE_SERIALIZATION_IMPL_SER_SHARED_PTR_TRACKER_H

#ifndef SST_INCLUDING_SERIALIZER_H
#warning \
    "The header file sst/core/serialization/impl/ser_shared_ptr_tracker.h should not be directly included as it is not part of the stable public API.  The file is included in sst/core/serialization/serializer.h"
#endif

#include "sst/core/output.h"

#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <utility>

namespace SST::Core::Serialization::pvt {

class ser_shared_ptr_packer
{
    // Map of ownership from std::shared_ptr or std::weak_ptr to a numeric tag representing the owner.
    // The std::owner_less<> functor compares two std::shared_ptr or std::weak_ptr instances with .owner_before() to
    // form an ordering which std::map uses to detect whether two std::shared_ptr or std::weak_ptr pointers have the
    // same owner. The keys are of type std::weak_ptr<const void> because any std::shared_ptr or std::weak_ptr can be
    // converted to them and their ownership can be compared with any other std::shared_ptr or std::weak_ptr.
    std::map<std::weak_ptr<const void>, size_t, std::owner_less<>> shared_ptr_map;

    // Running owner tag which starts at 1 and increments each time a new std::shared_ptr owner is found
    size_t owner_tag = 1;

public:
    // Get the tag associated with a shared pointer's owner, and whether it is new
    std::pair<size_t, bool> get_shared_ptr_owner_tag(const std::weak_ptr<const void>& ptr);
}; // class ser_shared_ptr_packer

class ser_shared_ptr_unpacker
{
    // Array of std::shared_ptr owners indexed by tag. When a std::shared_ptr or std::weak_ptr is deserialized, numeric
    // tags represent different owners. The first time a tag is seen, a std::shared_ptr<PARENT_TYPE> is allocated and
    // PARENT_TYPE is deserialized. This array keeps an copy of the std::shared_ptr<PARENT_TYPE> owner around for every
    // deserialization of std::shared_ptr or std::weak_ptr with the same ownership tag.
    // std::deque is used because it never has to move std::shared_ptr around after they are inserted.
    std::deque<std::shared_ptr<void>> shared_ptr_owners;

public:
    // Get the std::shared_ptr associated with a tag, and whether it is new
    std::pair<std::shared_ptr<void>&, bool> get_shared_ptr_owner(size_t tag);
}; // class ser_shared_ptr_unpacker

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_SER_SHARED_PTR_TRACKER_H
