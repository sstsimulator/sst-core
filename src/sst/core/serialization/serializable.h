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

#ifndef SST_CORE_SERIALIZATION_SERIALIZABLE_H
#define SST_CORE_SERIALIZATION_SERIALIZABLE_H

#include "sst/core/serialization/serializable_base.h"


namespace SST {
namespace Core {
namespace Serialization {


class serializable : public serializable_base
{
public:
    static constexpr uint32_t NullClsId = std::numeric_limits<uint32_t>::max();

    // virtual const char* cls_name() const = 0;

    // virtual void serialize_order(serializer& ser) = 0;

    // virtual uint32_t    cls_id() const             = 0;
    // virtual std::string serialization_name() const = 0;

    virtual ~serializable() {}
};

} // namespace Serialization
} // namespace Core
} // namespace SST

#include "sst/core/serialization/serialize_serializable.h"

#endif
