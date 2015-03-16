// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_SERIALIZATION_TYPES_H
#define SST_SERIALIZATION_TYPES_H

#include <boost/serialization/array.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

/* BWB: This is an ugly hack which will probably break serializing
   std::strings for MPI communication (which we thankfully don't
   generally do).  In Boost versions prior to 1.40, there was a bug
   which resulted in compile errors if a BOOST_CLASS_EXPORTed data
   structure contained an std::string.  We do use std::strings for
   classes which are serialized but not sent using MPI (like most
   elements), so that posed a problem.  Checkpointing pays no
   attention to this define, so it doesn't break those */
#if SST_BOOST_MPI_STD_STRING_BROKEN
BOOST_IS_MPI_DATATYPE(std::string)
#endif

#endif
