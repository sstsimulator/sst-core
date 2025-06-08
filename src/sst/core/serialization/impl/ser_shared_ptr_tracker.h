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

#include <deque>
#include <map>
#include <memory>
#include <utility>

namespace SST::Core::Serialization::pvt {

class ser_shared_ptr_packer
{
    // Map of ownership from std::shared_ptr or std::weak_ptr to a numeric tag representing the owner. The
    // std::owner_less<> functor compares two std::shared_ptr or std::weak_ptr instances with .owner_before() to
    // form an ordering which std:map uses to detect whether two std::shared_ptr or std::weak_ptr pointers have
    // the same owner. The keys are of type std::weak_ptr<const void> because any std::shared_ptr or std::weak_ptr
    // can be converted to it and its ownership can be compared with any other std::shared_ptr or std::weak_ptr.
    std::map<std::weak_ptr<const void>, size_t, std::owner_less<>> shared_ptr_map;

    size_t tag = 0;

protected:
    std::pair<size_t, bool> get_shared_ptr_tag(const std::weak_ptr<const void>& ptr)
    {
        // Look for the owner of the control block in the map, creating a new entry if it doesn't exist
        auto [it, is_new_tag] = shared_ptr_map.try_emplace(ptr, tag + 1);

        // If a new entry was added, increment the tag
        if ( is_new_tag ) ++tag;

        // Return the tag of the control block owner and whether it is new
        return { it->second, is_new_tag };
    }
}; // class ser_shared_ptr_packer


class ser_shared_ptr_unpacker
{
    // Deque of std::shared_ptr<void> owners indexed by a "restricted growth sequence" of tags. When
    // std::shared_ptr or std::weak_ptr is deserialized, numeric tags are used to represent owners. A
    // std::shared_ptr<PARENT_TYPE> is allocated and PARENT_TYPE is deserialized the first time a tag is seen.
    // The deque keeps a std::shared_ptr<void> copy of std::shared_ptr<PARENT_TYPE> around for every
    // deserialization of std::shared_ptr or std::weak_ptr with the same ownership tag.
    std::deque<std::shared_ptr<void>> shared_ptr_owners;

protected:
    std::pair<std::shared_ptr<void>&, bool> get_shared_ptr_owner(size_t tag)
    {
        if ( tag == 0 )
            Output::getDefaultObject().fatal(
                __LINE__, __FILE__, __func__, 1, "Deserialization Error: std::shared_ptr ownership tag must not be 0");

        size_t size = shared_ptr_owners.size();

        // If the tag has been seen before, return the owner associated with the tag
        if ( tag <= size ) return { shared_ptr_owners[tag - 1], false };

        // The first time a tag is seen, it must be 1 higher than the maximum seen so far
        if ( tag - size != 1 )
            Output::getDefaultObject().fatal(__LINE__, __FILE__, __func__, 1,
                "Deserialization Error: std::shared_ptr ownership tag is out of order");

        // Return a reference to a new std::shared_ptr<void> owner
        return { shared_ptr_owners.emplace_back(), true };
    }
}; // class ser_shared_ptr_unpacker

} // namespace SST::Core::Serialization::pvt

#endif // SST_CORE_SERIALIZATION_IMPL_SER_SHARED_PTR_TRACKER_H
