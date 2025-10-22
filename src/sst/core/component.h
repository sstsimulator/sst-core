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

#ifndef SST_CORE_COMPONENT_H
#define SST_CORE_COMPONENT_H

#include "sst/core/baseComponent.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/sst_types.h"

#include <map>

using namespace SST::Statistics;

namespace SST {
class SubComponent;

/**
 * Main component object for the simulation.
 *  All models inherit from this.
 */
class Component : public BaseComponent
{
public:
    SST_ELI_DECLARE_BASE(Component)
    // declare extern to limit compile times
    SST_ELI_DECLARE_CTOR_EXTERN(ComponentId_t,SST::Params&)
    // These categories will print in sst-info in the order they are
    // listed here
    SST_ELI_DECLARE_INFO_EXTERN(
        ELI::ProvidesCategory,
        ELI::ProvidesParams,
        ELI::ProvidesPorts,
        ELI::ProvidesSubComponentSlots,
        ELI::ProvidesStats,
        ELI::ProvidesCheckpointable,
        ELI::ProvidesProfilePoints,
        ELI::ProvidesAttributes)

    /** Constructor. Generally only called by the factory class.
        @param id Unique component ID
    */
    explicit Component(ComponentId_t id);
    virtual ~Component() override = default;


    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Component)

protected:
    friend class SubComponent;
    Component() = default; // For Serialization only
};

} // namespace SST

// These macros allow you to register a base class for a set of
// components that have the same (or substantially the same) ELI
// information.  ELI information can be defined in the base class, and
// will be inherited by the child classes.
#define SST_ELI_REGISTER_COMPONENT_BASE(cls) \
    SST_ELI_DECLARE_NEW_BASE(SST::Component,::cls)                 \
    SST_ELI_NEW_BASE_CTOR(SST::ComponentId_t,SST::Params&)

#define SST_ELI_REGISTER_COMPONENT_DERIVED_BASE(cls, base) \
    SST_ELI_DECLARE_NEW_BASE(::base,::cls)                               \
    SST_ELI_NEW_BASE_CTOR(SST::ComponentId_t,SST::Params&)

// 'x' is needed because you can't pass ##__VAR_ARGS__ as the first
// item to the macro or you end up with a leading comma that won't
// compile.
#define ELI_GET_COMPONENT_DEFAULT(x, arg1, ...) arg1

#define SST_ELI_REGISTER_COMPONENT(cls, lib, name, version, desc, cat, ...) \
    SST_ELI_REGISTER_DERIVED(ELI_GET_COMPONENT_DEFAULT(,##__VA_ARGS__,SST::Component),cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)                                                \
    SST_ELI_CATEGORY_INFO(cat)

#endif // SST_CORE_COMPONENT_H
