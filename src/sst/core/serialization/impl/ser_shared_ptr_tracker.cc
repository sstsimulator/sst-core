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

#include "sst/core/serialization/serializer.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace SST::Core::Serialization::pvt {

// Get the tag associated with a shared pointer's owner, and whether it is new
std::pair<size_t, bool>
ser_shared_ptr_packer::get_shared_ptr_owner_tag(const std::weak_ptr<const void>& ptr)
{
    // If the std::shared_ptr or std::weak_ptr is empty, a tag of 0 is returned
    if ( ptr.use_count() == 0 ) return { 0, false };

    // Look for the owner of the control block in the map, creating a new owner tag if it doesn't exist
    auto [it, is_new_tag] = shared_ptr_map.try_emplace(ptr, owner_tag);

    // If a new entry was added, increment the owner tag for the next time a new tag is added
    if ( is_new_tag ) ++owner_tag;

    // Return the tag of the control block's owner and whether it is new
    return { it->second, is_new_tag };
}

// Get the std::shared_ptr associated with a tag, and whether it is new
std::pair<std::shared_ptr<void>&, bool>
ser_shared_ptr_unpacker::get_shared_ptr_owner(size_t tag)
{
    // Should never get here if the tag is 0
    if ( tag == 0 ) throw std::invalid_argument("Deserialization error: std::shared_ptr ownership tag must not be 0");

    // Get the current largest tag
    size_t num_owners = shared_ptr_owners.size();

    // If the tag has been seen before, return the owner associated with the tag
    if ( --tag < num_owners ) return { shared_ptr_owners[tag], false };

    // The first time a new tag is seen, it must be 1 higher than the maximum seen so far (restricted growth sequence)
    if ( tag != num_owners )
        throw std::invalid_argument("Deserialization error: std::shared_ptr ownership tag is out of order");

    // Return a reference to a new std::shared_ptr<void> owner for the tag
    return { shared_ptr_owners.emplace_back(), true };
}

} // namespace SST::Core::Serialization::pvt
