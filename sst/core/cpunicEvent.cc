// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include <boost/mpi.hpp>

#include "sst/core/cpunicEvent.h"

BOOST_CLASS_EXPORT(SST::CPUNicEvent)

// This is an optimization for fixed-size types to avoid mem copies.
// It must NOT be used for variable-size types like vectors!
// BOOST_IS_MPI_DATATYPE(SST::CPUNicEvent)
