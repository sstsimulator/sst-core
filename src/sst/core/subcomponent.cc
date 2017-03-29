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

#include <sst_config.h>
#include <sst/core/subcomponent.h>
#include <sst/core/factory.h>

namespace SST {

bool
SubComponent::doesComponentInfoStatisticExist(const std::string &statisticName)
{
    return Factory::getFactory()->DoesSubComponentInfoStatisticNameExist(my_info->getType(), statisticName);
}

uint8_t SubComponent::getComponentInfoStatisticEnableLevel(const std::string &statisticName)
{
    const std::string& type = parent->my_info->getType();
    /* TODO:  SubComponent, not component stat? */
    return Factory::getFactory()->GetComponentInfoStatisticEnableLevel(type, statisticName);
}

std::string SubComponent::getComponentInfoStatisticUnits(const std::string &statisticName)
{
    const std::string& type = parent->my_info->getType();
    /* TODO:  SubComponent, not component stat? */
    return Factory::getFactory()->GetComponentInfoStatisticUnits(type, statisticName);
}


} // namespace SST
