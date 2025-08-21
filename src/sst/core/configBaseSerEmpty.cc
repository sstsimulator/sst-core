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

#include "sst/core/configBase.h"

#include "sst/core/serialization/serialize.h"
#include "sst/core/warnmacros.h"

#include <string>

#define INSTANTIATE_SERIALIZE_DATA(type)                                                             \
    template <>                                                                                      \
    void option_serialize_data(SST::Core::Serialization::serializer& UNUSED(ser), type& UNUSED(val)) \
    {}


namespace SST::Impl {

INSTANTIATE_SERIALIZE_DATA(bool);
INSTANTIATE_SERIALIZE_DATA(int);
INSTANTIATE_SERIALIZE_DATA(std::string);


} // namespace SST::Impl
