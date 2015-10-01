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
//
#include "sst_config.h"

#include "sst/core/serialization.h"
#include "sst/core/interfaces/simpleNetwork.h"

namespace SST {
namespace Interfaces {

template<class Archive>
void SimpleNetwork::Request::serialize(Archive & ar, const unsigned int version )
{
    // std::cout << "Request::serialize enter" << std::endl;
    ar & BOOST_SERIALIZATION_NVP(dest);
    ar & BOOST_SERIALIZATION_NVP(src);
    ar & BOOST_SERIALIZATION_NVP(vn);
    ar & BOOST_SERIALIZATION_NVP(size_in_bits);
    ar & BOOST_SERIALIZATION_NVP(head);
    ar & BOOST_SERIALIZATION_NVP(tail);
    ar & BOOST_SERIALIZATION_NVP(payload);
    ar & BOOST_SERIALIZATION_NVP(trace);
    ar & BOOST_SERIALIZATION_NVP(traceID);
    // std::cout << "Request::serialize exit" << std::endl;
}

template<class Archive>
void SimpleNetwork::Mapping::serialize(Archive & ar, const unsigned int version )
{
    ar & BOOST_SERIALIZATION_NVP(data);
}


} // namespace Interfaces
} // namespace SST
    
SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Interfaces::SimpleNetwork::Request::serialize);
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Interfaces::SimpleNetwork::Request);

SST_BOOST_SERIALIZATION_INSTANTIATE(SST::Interfaces::SimpleNetwork::Mapping::serialize);
BOOST_CLASS_EXPORT_IMPLEMENT(SST::Interfaces::SimpleNetwork::Mapping);


