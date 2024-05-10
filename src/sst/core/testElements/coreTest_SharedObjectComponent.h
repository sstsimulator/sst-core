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

#ifndef SST_CORE_CORETEST_SHAREDOBJECT_H
#define SST_CORE_CORETEST_SHAREDOBJECT_H

#include "sst/core/component.h"
#include "sst/core/output.h"
#include "sst/core/shared/sharedArray.h"
#include "sst/core/shared/sharedMap.h"
#include "sst/core/shared/sharedSet.h"

namespace SST {
namespace CoreTestSharedObjectsComponent {

// Class so that we can check to see if the equivalence check works
// for sets
struct setItem : public SST::Core::Serialization::serializable
{
    int key;
    int value;

    setItem() : key(0), value(0) {}

    setItem(int key, int value) : key(key), value(value) {}

    bool operator<(const setItem& lhs) const { return key < lhs.key; }

    bool operator==(const setItem& lhs) const
    {
        if ( key != lhs.key ) return false;
        if ( value != lhs.value ) return false;
        return true;
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST_SER(key);
        SST_SER(value);
    }

    ImplementSerializable(SST::CoreTestSharedObjectsComponent::setItem);
};

class coreTestSharedObjectsComponent : public SST::Component
{
public:
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        coreTestSharedObjectsComponent,
        "coreTestElement",
        "coreTestSharedObjectsComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Test for SharedObjects",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "object_type", "Type of object to test ( array | map | set )", "array"},
        { "num_entities", "Number of entities in the sim", "12"},
        { "myid", "ID Number (0 <= myid < num_entities)", nullptr},
        { "full_initialization", "If true, id 0 will initialize whole array, otherwise each id will contribute", "true"},
        { "multiple_initializers", "If doing full_initialization, this will cause ID N-1 to also initialize array", "false"},
        { "conflicting_write", "Controls whether a conflicting write is done when full_initialization and multiple_initializers are turned on (otherwise it has no effect)", "false"},
        { "verify_mode", "Sets verify mode for SharedArray ( FE | INIT | NONE )", "INIT" },
        { "late_write", "Controls whether a late write is done", "false" },
        { "publish", "Controls whether publish() is called or not", "true"},
        { "double_initialize", "If true, initialize() will be called twice", "false" },
        { "late_initialize", "If true, initialize() will be called during setup instead of in constructor", "false" },
        { "checkpoint", "If true, SharedObject state will be printed in setup() and finish()", "false" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS(
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    coreTestSharedObjectsComponent(SST::ComponentId_t id, SST::Params& params);
    ~coreTestSharedObjectsComponent() {}

    void init(unsigned int phase) override;
    void setup() override;
    void finish() override;
    void complete(unsigned int phase) override;

    bool tick(SST::Cycle_t);

    coreTestSharedObjectsComponent() : Component() {} // For serialization ONLY
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::CoreTestSharedObjectsComponent::coreTestSharedObjectsComponent)

private:
    Output out;

    bool test_array;
    bool test_bool_array;
    bool test_map;
    bool test_set;

    int myid;
    int num_entities;

    int  count;
    bool check;
    bool late_write;
    bool pub;
    bool late_initialize;
    bool checkpoint;

    Shared::SharedArray<int>    array;
    Shared::SharedArray<bool>   bool_array;
    Shared::SharedMap<int, int> map;
    Shared::SharedSet<setItem>  set;
};

} // namespace CoreTestSharedObjectsComponent
} // namespace SST

#endif // SST_CORE_CORETEST_SHAREDOBJECT_H
