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


#include "sst_config.h"
#include "sst/core/serialization.h"

#include "sst/core/introspectAction.h"
//#include "sst/core/event.h"

namespace SST {


void
IntrospectAction::execute(void)
{
    Event* event = NULL;
    //executing introspector-writer's mpi collective communication function
    ( *m_handler )( event );
}

    
template<class Archive>
void
IntrospectAction::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
}

} // namespace SST


SST_BOOST_SERIALIZATION_INSTANTIATE(SST::IntrospectAction::serialize)

BOOST_CLASS_EXPORT_IMPLEMENT(SST::IntrospectAction)
