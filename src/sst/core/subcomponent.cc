// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/subcomponent.h"
#include "sst/core/factory.h"

namespace SST {

SST_ELI_DEFINE_INFO_EXTERN(SubComponent)
SST_ELI_DEFINE_CTOR_EXTERN(SubComponent)

#ifndef SST_ENABLE_PREVIEW_BUILD
SubComponent::SubComponent(Component* parent) :
    BaseComponent(parent->getCurrentlyLoadingSubComponentID()),
    parent(parent)
{
    loadedWithLegacyAPI = true;
}
#endif

SubComponent::SubComponent(ComponentId_t id) :
#ifndef SST_ENABLE_PREVIEW_BUILD
    BaseComponent(id),
    parent(getTrueComponentPrivate())
#else
    BaseComponent(id)
#endif
{}


#ifndef SST_ENABLE_PREVIEW_BUILD
SubComponent*
SubComponent::loadSubComponent(const std::string& type, Params& params)
{
    return BaseComponent::loadSubComponent(type, getTrueComponentPrivate(), params);
}
#endif

} // namespace SST
