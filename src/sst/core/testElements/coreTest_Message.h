// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORETEST_MESSAGE_H
#define SST_CORE_CORETEST_MESSAGE_H

namespace SST {
namespace CoreTestMessageGeneratorComponent {

class coreTestMessage : public SST::Event
{
public:
    coreTestMessage() : SST::Event() {}

public:
    void serialize_order(SST::Core::Serialization::serializer& ser) override { Event::serialize_order(ser); }

    ImplementSerializable(SST::CoreTestMessageGeneratorComponent::coreTestMessage);
};

} // namespace CoreTestMessageGeneratorComponent
} // namespace SST

#endif // SST_CORE_CORETEST_MESSAGE_H
