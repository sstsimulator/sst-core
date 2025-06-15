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

#include <cstdint>
#include <cstring>
#include <string>

namespace SST::Core::Serialization::pvt {

uintptr_t
ser_unpacker::check_pointer_unpack(uintptr_t ptr)
{
    auto it = ser_pointer_map.find(ptr);
    if ( it != ser_pointer_map.end() ) return it->second;

    // Keep a copy of the ptr in case we have a split report
    split_key = ptr;
    return 0;
}

void
ser_unpacker::unpack_string(std::string& str)
{
    size_t size;
    unpack(size);
    str.resize(size);
    memcpy(str.data(), buf_next(size), size);
}

} // namespace SST::Core::Serialization::pvt
