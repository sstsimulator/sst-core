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

#ifndef SST_CORE_SERIALIZATION_ARCHIVE_H
#define SST_CORE_SERIALIZATION_ARCHIVE_H

#if defined(SST_CORE_SERIALIZATION_ELEMENT_H) || defined(SST_CORE_SERIALIZATION_CORE_H)
#error "Include only one serialization/ header file"
#endif

#include <boost/archive/polymorphic_xml_iarchive.hpp>
#include <boost/archive/polymorphic_xml_oarchive.hpp>
#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>
#include <boost/mpi.hpp>

#endif // #ifndef SST_CORE_SERIALIZATION_ARCHIVE_H
