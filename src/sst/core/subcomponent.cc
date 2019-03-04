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
#include <sst/core/subcomponent.h>
#include <sst/core/factory.h>

namespace SST {

//SST_ELI_DEFINE_INFO_EXTERN(SubComponent)

bool SubComponent::addInfo(const std::string& elemlib, const std::string& elem, BuilderInfo* info){
  std::cout << "Adding " << elemlib << " " << elem << std::endl;
  return ::SST::ELI::InfoDatabase::getLibrary<__LocalEliBase>(elemlib)->addInfo(elem,info);
}

SST_ELI_DEFINE_CTOR_EXTERN(SubComponent)

bool
SubComponent::doesComponentInfoStatisticExist(const std::string &statisticName) const
{
    return Factory::getFactory()->DoesSubComponentInfoStatisticNameExist(my_info->getType(), statisticName);
}

} // namespace SST
