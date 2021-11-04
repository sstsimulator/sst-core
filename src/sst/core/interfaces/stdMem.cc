// -*- mode: c++ -*-
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
//
#include "sst_config.h"

#include "sst/core/interfaces/stdMem.h"

std::atomic<SST::Experimental::Interfaces::StandardMem::Request::id_t>
    SST::Experimental::Interfaces::StandardMem::Request::main_id(0);

std::atomic<SST::Interfaces::StandardMem::Request::id_t> SST::Interfaces::StandardMem::Request::main_id(0);
