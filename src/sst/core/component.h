// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
        ELI::ProvidesProfilePoints,
        ELI::ProvidesAttributes)

    /** Constructor. Generally only called by the factory class.
        @param id Unique component ID
    */
    Component(ComponentId_t id);
    virtual ~Component();

    /** Register as a primary component, which allows the component to
        specify when it is and is not OK to end simulation.  The
        simulator will not end simulation naturally (through use of
        the Exit object) while any primary component has specified
        primaryComponentDoNotEndSim().  However, it is still possible
        for Actions other than Exit to end simulation.  Once all
        primary components have specified
        primaryComponentOKToEndSim(), the Exit object will trigger and
        end simulation.

    This must be called during simulation wireup (i.e during the
    constructor for the component).  By default, the state of the
    primary component is set to OKToEndSim.

    If no component registers as a primary component, then the
    Exit object will not be used for that simulation and
    simulation termination must be accomplished through some other
    mechanism (e.g. --stopAt flag, or some other Action object).

        @sa Component::primaryComponentDoNotEndSim()
        @sa Component::primaryComponentOKToEndSim()
    */
    void registerAsPrimaryComponent();

    /** Tells the simulation that it should not exit.  The component
    will remain in this state until a call to
    primaryComponentOKToEndSim().

        @sa Component::registerAsPrimaryComponent()
        @sa Component::primaryComponentOKToEndSim()
    */
    void primaryComponentDoNotEndSim();

    /** Tells the simulation that it is now OK to end simulation.
    Simulation will not end until all primary components have
    called this function.

        @sa Component::registerAsPrimaryComponent()
        @sa Component::primaryComponentDoNotEndSim()
    */
    void primaryComponentOKToEndSim();

protected:
    friend class SubComponent;
};

} // namespace SST

// These macros allow you to register a base class for a set of
// components that have the same (or substantially the same) ELI
// information.  ELI information can be defined in the base class, and
// will be inherited by the child classes.
#define SST_ELI_REGISTER_COMPONENT_BASE(cls)       \
    SST_ELI_DECLARE_NEW_BASE(SST::Component,::cls) \
    SST_ELI_NEW_BASE_CTOR(SST::ComponentId_t,SST::Params&)

#define SST_ELI_REGISTER_COMPONENT_DERIVED_BASE(cls, base) \
    SST_ELI_DECLARE_NEW_BASE(::base,::cls)                 \
    SST_ELI_NEW_BASE_CTOR(SST::ComponentId_t,SST::Params&)

// 'x' is needed because you can't pass ##__VAR_ARGS__ as the first
// item to the macro or you end up with a leading comma that won't
// compile.
#define ELI_GET_COMPONENT_DEFAULT(x, arg1, ...) arg1

#define SST_ELI_REGISTER_COMPONENT(cls, lib, name, version, desc, cat, ...)                                                          \
    SST_ELI_REGISTER_DERIVED(ELI_GET_COMPONENT_DEFAULT(,##__VA_ARGS__,SST::Component),cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
    SST_ELI_CATEGORY_INFO(cat)

#endif // SST_CORE_COMPONENT_H
