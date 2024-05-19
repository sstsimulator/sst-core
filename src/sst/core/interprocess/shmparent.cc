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

/* This exists only to test shmparent and shmchild during sst-core compilation */

#include "sst_config.h"

#include "sst/core/interprocess/shmparent.h"

#include "sst/core/interprocess/shmchild.h"
#include "sst/core/interprocess/tunneldef.h"

typedef SST::Core::Interprocess::TunnelDef<int, int> testtunnel;

template class SST::Core::Interprocess::SHMParent<testtunnel>;
template class SST::Core::Interprocess::SHMChild<testtunnel>;
