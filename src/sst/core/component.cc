// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <string>

#include <sst/core/component.h>
#include <sst/core/exit.h>
#include <sst/core/simulation.h>
#include <sst/core/factory.h>

using namespace SST::Statistics;

namespace SST {

Component::Component(ComponentId_t id) : BaseComponent(),
    id(id)
{
    my_info = sim->getComponentInfo(id);
}


Component::~Component()
{
}



bool Component::registerExit()
{
    int thread = getSimulation()->getRank().thread;
    return getSimulation()->getExit()->refInc( getId(), thread );
}

bool Component::unregisterExit()
{
    int thread = getSimulation()->getRank().thread;
    return getSimulation()->getExit()->refDec( getId(), thread );
}

void
Component::registerAsPrimaryComponent()
{
    // Nop for now.  Will put in complete semantics later
}

void
Component::primaryComponentDoNotEndSim()
{
    int thread = getSimulation()->getRank().thread;
    getSimulation()->getExit()->refInc( getId(), thread );
}

void
Component::primaryComponentOKToEndSim()
{
    int thread = getSimulation()->getRank().thread;
    getSimulation()->getExit()->refDec( getId(), thread );
}



bool Component::doesComponentInfoStatisticExist(const std::string &statisticName) const
{
    const std::string& type = my_info->getType();
    return Factory::getFactory()->DoesComponentInfoStatisticNameExist(type, statisticName);
}


} // namespace SST


