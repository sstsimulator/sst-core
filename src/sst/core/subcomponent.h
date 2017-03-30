// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_SUBCOMPONENT_H
#define SST_CORE_SUBCOMPONENT_H

#include <sst/core/baseComponent.h>
#include <sst/core/component.h>

namespace SST {


/**
   SubComponent is a class loadable through the factory which allows
   dynamic functionality to be added to a Component.  The
   SubComponent API is nearly identical to the Component API and all
   the calls are proxied to the parent Compoent.
*/
class SubComponent : public Module, public BaseComponent {

public:
	SubComponent(Component* parent) : BaseComponent(), parent(parent) {
        my_info = parent->currentlyLoadingSubComponent;
    };
	virtual ~SubComponent() {};

    /** Used during the init phase.  The method will be called each phase of initialization.
     Initialization ends when no components have sent any data. */
    virtual void init(unsigned int phase __attribute__((unused))) {}
    /** Called after all components have been constructed and inialization has
	completed, but before simulation time has begun. */
    virtual void setup( ) { }
    /** Called after simulation completes, but before objects are
        destroyed. A good place to print out statistics. */
    virtual void finish( ) { }

protected:
    Component* const parent;

    Component* getTrueComponent() final { return parent; }
    BaseComponent* getStatisticOwner() final {
        /* If our ID == parent ID, then we're a legacy subcomponent that doesn't own stats. */
        if ( this->getId() == parent->getId() )
            return parent;
        return this;
    }

    /* Deprecate?   Old ELI style*/
    SubComponent* loadSubComponent(std::string type, Params& params) {
        return parent->loadSubComponent(type, parent, params);
    }

    // Does the statisticName exist in the ElementInfoStatistic
    virtual bool doesComponentInfoStatisticExist(const std::string &statisticName) final;

private:
    /** Component's type, set by the factory when the object is created.
        It is identical to the configuration string used to create the
        component. I.e. the XML "<component id="aFoo"><foo>..." would
        set component::type to "foo" */
    friend class Component;

};
} //namespace SST


#endif // SST_CORE_SUBCOMPONENT_H
