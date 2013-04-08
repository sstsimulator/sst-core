// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/serialization/core.h"

#include "sst/core/element.h"
#include "memEvent.h"

using namespace SST;
using namespace SST::Interfaces;

uint64_t SST::Interfaces::MemEvent::main_id = 0;

BOOST_CLASS_EXPORT(MemEvent)
