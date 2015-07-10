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


#include <sst_config.h>
#include <sst/core/serialization.h>

#include <boost/serialization/binary_object.hpp>

#include <sst/core/sharedRegion.h>
#include <sst/core/sharedRegionImpl.h>


template<class Archive>
void SST::RegionInfo::RegionMergeInfo::serialize(Archive & ar, const unsigned int version )
{
    ar & BOOST_SERIALIZATION_NVP(rank);
    ar & BOOST_SERIALIZATION_NVP(key);
}
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::RegionInfo::RegionMergeInfo::serialize);
BOOST_CLASS_EXPORT(SST::RegionInfo::RegionMergeInfo);


template<class Archive>
void SST::RegionInfo::BulkMergeInfo::serialize(Archive & ar, const unsigned int version )
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::RegionInfo::RegionMergeInfo);
    ar & BOOST_SERIALIZATION_NVP(length);
    if ( Archive::is_loading::value ) {
        data = malloc(length);
    }
    ar & boost::serialization::make_binary_object(data, length);
}
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::RegionInfo::BulkMergeInfo::serialize);
BOOST_CLASS_EXPORT(SST::RegionInfo::BulkMergeInfo);


template<class Archive>
void SST::RegionInfo::ChangeSetMergeInfo::serialize(Archive & ar, const unsigned int version )
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::RegionInfo::RegionMergeInfo);
    ar & BOOST_SERIALIZATION_NVP(changeSets);
}
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::RegionInfo::ChangeSetMergeInfo::serialize);
BOOST_CLASS_EXPORT(SST::RegionInfo::ChangeSetMergeInfo);




template<class Archive>
void SST::SharedRegionMerger::ChangeSet::serialize(Archive & ar, const unsigned int version )
{
    ar & BOOST_SERIALIZATION_NVP(offset);
    ar & BOOST_SERIALIZATION_NVP(length);
    if ( Archive::is_loading::value ) {
        data = (const uint8_t*)malloc(length);
    }
    ar & boost::serialization::make_binary_object((void*)data, length);
}
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::SharedRegionMerger::ChangeSet::serialize);
BOOST_CLASS_EXPORT_IMPLEMENT(SST::SharedRegionMerger::ChangeSet);

