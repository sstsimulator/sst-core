// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/profile/profiletool.h"

#include "sst/core/sst_types.h"

#include <atomic>

namespace SST {
namespace Profile {

SST_ELI_DEFINE_INFO_EXTERN(ProfileTool)
SST_ELI_DEFINE_CTOR_EXTERN(ProfileTool)


ProfileTool::ProfileTool(const std::string& name) : name(name) {}

} // namespace Profile
} // namespace SST
