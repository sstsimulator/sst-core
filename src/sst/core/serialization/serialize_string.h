// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_SERIALIZE_STRING_H
#define SST_CORE_SERIALIZATION_SERIALIZE_STRING_H

#include "sst/core/serialization/serializer.h"

namespace SST {
namespace Core {
namespace Serialization {

template <>
class serialize<std::string>
{
public:
    void operator()(std::string& str, serializer& ser) { ser.string(str); }
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_SERIALIZE_STRING_H
