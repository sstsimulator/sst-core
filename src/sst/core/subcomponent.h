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

#ifndef SST_CORE_SUBCOMPONENT_H
#define SST_CORE_SUBCOMPONENT_H

#include "sst/core/baseComponent.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/module.h"
#include "sst/core/warnmacros.h"

namespace SST {

/**
   SubComponent is a class loadable through the factory which allows
   dynamic functionality to be added to a Component.  The
   SubComponent API is nearly identical to the Component API and all
   the calls are proxied to the parent Component.
*/
class SubComponent : public BaseComponent
{

public:
    SST_ELI_DECLARE_BASE(SubComponent)
    // declare extern to limit compile times
    SST_ELI_DECLARE_CTOR_EXTERN(ComponentId_t)
    // These categories will print in sst-info in the order they are
    // listed here
    SST_ELI_DECLARE_INFO_EXTERN(
        ELI::ProvidesInterface,
        ELI::ProvidesParams,
        ELI::ProvidesPorts,
        ELI::ProvidesSubComponentSlots,
        ELI::ProvidesStats,
        ELI::ProvidesProfilePoints,
        ELI::ProvidesAttributes)

    SubComponent(ComponentId_t id);

    virtual ~SubComponent() {};

    /** Used during the init phase.  The method will be called each phase of initialization.
     Initialization ends when no components have sent any data. */
    virtual void init(unsigned int UNUSED(phase)) override {}
    /** Called after all components have been constructed and initialization has
    completed, but before simulation time has begun. */
    virtual void setup() override {}
    /** Called after simulation completes, but before objects are
        destroyed. A good place to print out statistics. */
    virtual void finish() override {}

protected:
    // For serialization only
    SubComponent();
    void serialize_order(SST::Core::Serialization::serializer& ser) override;

    friend class Component;
    ImplementSerializable(SST::SubComponent)
};

} // namespace SST

// New way to register subcomponents.  Must register an interface
// (API) first, then you can register a subcomponent that implements
// it
#define SST_ELI_REGISTER_SUBCOMPONENT_API(cls, ...)   \
    SST_ELI_DECLARE_NEW_BASE(SST::SubComponent,::cls) \
    SST_ELI_NEW_BASE_CTOR(SST::ComponentId_t,SST::Params&,##__VA_ARGS__)

#define SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(cls, base, ...) \
    SST_ELI_DECLARE_NEW_BASE(::base,::cls)                        \
    SST_ELI_NEW_BASE_CTOR(SST::ComponentId_t,SST::Params&,##__VA_ARGS__)

#define SST_ELI_REGISTER_SUBCOMPONENT(cls, lib, name, version, desc, interface)         \
    SST_ELI_REGISTER_DERIVED(::interface,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
    SST_ELI_INTERFACE_INFO(#interface)

#endif // SST_CORE_SUBCOMPONENT_H
