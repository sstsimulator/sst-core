// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_COMPONENT_H
#define SST_CORE_COMPONENT_H

#include "sst/core/sst_types.h"

#include <map>

#include "sst/core/baseComponent.h"
#include "sst/core/eli/elementinfo.h"

using namespace SST::Statistics;

namespace SST {
class SubComponent;

/**
 * Main component object for the simulation.
 *  All models inherit from this.
 */
class Component: public BaseComponent {
public:
    SST_ELI_DECLARE_BASE(Component)
    //declare extern to limit compile times
    SST_ELI_DECLARE_CTOR_EXTERN(ComponentId_t,SST::Params&)
    SST_ELI_DECLARE_INFO_EXTERN( 
      ELI::ProvidesParams,
      ELI::ProvidesSubComponentSlots,
      ELI::ProvidesPorts,
      ELI::ProvidesStats,
      ELI::ProvidesCategory)

    /** Constructor. Generally only called by the factory class.
        @param id Unique component ID
    */
    Component( ComponentId_t id );
    virtual ~Component();


    /** Register that the simulation should not end until this
        component says it is OK to. Calling this function (generally
        done in Component::setup() or in component constructor)
        increments a global counter. Calls to
        Component::unregisterExit() decrements the counter. The
        simulation cannot end unless this counter reaches zero, or the
        simulation time limit is reached. This counter is synchronized
        periodically with the other nodes.

        @sa Component::unregisterExit()
    */
    bool registerExit() __attribute__ ((deprecated("registerExit is deprecated and will be removed in SST version 10.0.  Please use registerAsPrimaryComponent() and primaryComponentDoNotEndSim() instead.")));

    /** Indicate permission for the simulation to end. This function is
        the mirror of Component::registerExit(). It decrements the
        global counter, which, upon reaching zero, indicates that the
        simulation can terminate. @sa Component::registerExit() */
    bool unregisterExit() __attribute__ ((deprecated("unregisterExit is deprecated and will be removed in SST version 10.0.  Please use primaryComponentOKToEndSim() instead.")));

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
    Component() {} // Unused, but previously available

};

} //namespace SST

#define SST_ELI_REGISTER_COMPONENT(cls,lib,name,version,desc,cat)   \
    SST_ELI_REGISTER_DERIVED(SST::Component,cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
    SST_ELI_CATEGORY_INFO(cat)

#endif // SST_CORE_COMPONENT_H
