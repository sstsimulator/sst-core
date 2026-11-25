// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/serialization/serializer.h"

#include <string>

namespace SST::Core::Serialization::pvt {

template <typename PACKER>
void
ser_pack_ops<PACKER>::pack_string(const std::string& str)
{
    pack_buffer(str.data(), str.size());
}

template void ser_pack_ops<ser_packer>::pack_string(const std::string&);
template void ser_pack_ops<ser_packer_growable>::pack_string(const std::string&);

} // namespace SST::Core::Serialization::pvt
