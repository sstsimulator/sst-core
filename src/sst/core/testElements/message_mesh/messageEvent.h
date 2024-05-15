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

#ifndef SST_CORE_CORETEST_MESSAGEMESH_MESSAGEEVENT_H
#define SST_CORE_CORETEST_MESSAGEMESH_MESSAGEEVENT_H

#include "sst/core/event.h"

namespace SST {
namespace CoreTest {
namespace MessageMesh {

class MessageEvent : public SST::Event
{
public:
    void serialize_order(SST::Core::Serialization::serializer& ser) override { Event::serialize_order(ser); }

    ImplementSerializable(SST::CoreTest::MessageMesh::MessageEvent);
};

} // namespace MessageMesh
} // namespace CoreTest
} // namespace SST

#endif // SST_CORE_CORETEST_MESSAGEMESH_MESSAGEEVENT_H
