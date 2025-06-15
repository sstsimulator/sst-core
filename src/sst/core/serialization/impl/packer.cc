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

#include <cstring>
#include <string>

namespace SST::Core::Serialization::pvt {

void
ser_packer::pack_string(std::string& str)
{
    size_t size = str.size();
    pack(size);
    memcpy(buf_next(size), str.data(), size);
}

} // namespace SST::Core::Serialization::pvt
