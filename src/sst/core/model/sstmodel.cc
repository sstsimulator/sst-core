// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/model/sstmodel.h"

#include "sst/core/factory.h"

namespace SST {

SST_ELI_DEFINE_INFO_EXTERN(SSTModelDescription)
SST_ELI_DEFINE_CTOR_EXTERN(SSTModelDescription)

SSTModelDescription::SSTModelDescription() {}

bool
SSTModelDescription::isElementParallelCapable(const std::string& type)
{
    return Factory::getFactory()->getSimpleInfo<SSTModelDescription, 0, bool>(type);
}

const std::vector<std::string>&
SSTModelDescription::getElementSupportedExtensions(const std::string& type)
{
    return Factory::getFactory()->getSimpleInfo<SSTModelDescription, 1, std::vector<std::string>>(type);
}


} // namespace SST
