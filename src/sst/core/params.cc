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
//

#include <sst_config.h>
#include <sst/core/params.h>

#include <map>
#include <vector>
#include <string>


std::map<std::string, uint32_t> SST::Params::keyMap;
std::vector<std::string> SST::Params::keyMapReverse;
SST::Core::ThreadSafe::Spinlock SST::Params::keyLock;
uint32_t SST::Params::nextKeyID;
bool SST::Params::g_verify_enabled = false;
