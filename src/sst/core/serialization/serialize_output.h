// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_OUTPUT_H
#define SST_CORE_SERIALIZATION_SERIALIZE_OUTPUT_H

#include "sst/core/output.h"
#include "sst/core/serialization/serializer.h"

namespace SST {
namespace Core {
namespace Serialization {

template <>
class serialize_impl<Output>
{
public:
    void operator()(Output& out, serializer& ser) { out.serialize_order(ser); }
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif /* SST_CORE_SERIALIZATION_SERIALIZE_OUTPUT_H */