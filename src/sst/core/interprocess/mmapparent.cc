// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/* This exists only to test mmapparent during sst-core compilation */

#include "sst_config.h"

#include "sst/core/interprocess/mmapparent.h"

#include "sst/core/interprocess/tunneldef.h"

using testtunnel = SST::Core::Interprocess::TunnelDef<int, int>;
template class SST::Core::Interprocess::MMAPParent<testtunnel>;
