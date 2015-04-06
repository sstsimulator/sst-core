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

#ifndef SST_CORE_SERIALIZATION_CORE_H
#define SST_CORE_SERIALIZATION_CORE_H

#if defined(SST_CORE_SERIALIZATION_ELEMENT_H)
#error "Include only one serialization/ header file"
#endif

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wsign-compare"  // IGNORES Sign Compare WARNING IN boost include/boost/mpl/print.hpp line 61
#endif

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wc++11-extensions"
#pragma clang diagnostic ignored "-Wdivision-by-zero"  // IGNORES Divide By Zero WARNING IN boost include/boost/mpl/print.hpp line 50 
#endif

#if SST_WANT_POLYMORPHIC_ARCHIVE
#include <boost/archive/polymorphic_iarchive.hpp>
#include <boost/archive/polymorphic_oarchive.hpp>
#else
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#endif

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>

#include <boost/serialization/type_info_implementation.hpp>
#include <boost/serialization/extended_type_info_no_rtti.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif

#include "sst/core/serialization/types.h"

#if SST_WANT_POLYMORPHIC_ARCHIVE
#ifdef HAVE_MPI
#define SST_BOOST_SERIALIZATION_INSTANTIATE(func)                       \
    template void                                                       \
    func<boost::archive::polymorphic_iarchive>(                         \
                                    boost::archive::polymorphic_iarchive & ar, \
                                    const unsigned int file_version);   \
    template void                                                       \
    func<boost::archive::polymorphic_oarchive>(                         \
                                    boost::archive::polymorphic_oarchive & ar, \
                                    const unsigned int file_version);   
#else
#define SST_BOOST_SERIALIZATION_INSTANTIATE(func)                       \
    template void                                                       \
    func<boost::archive::polymorphic_iarchive>(                         \
                                    boost::archive::polymorphic_iarchive & ar, \
                                    const unsigned int file_version);   \
    template void                                                       \
    func<boost::archive::polymorphic_oarchive>(                         \
                                    boost::archive::polymorphic_oarchive & ar, \
                                    const unsigned int file_version); 
#endif
#else
#define SST_BOOST_SERIALIZATION_INSTANTIATE(func)                       \
    template void                                                       \
    func<boost::archive::binary_iarchive>(                              \
                                    boost::archive::binary_iarchive & ar, \
                                    const unsigned int file_version);   \
    template void                                                       \
    func<boost::archive::binary_oarchive>(                              \
                                    boost::archive::binary_oarchive & ar, \
                                    const unsigned int file_version);
#endif

#endif // #ifndef SST_CORE_SERIALIZATION_CORE_H
