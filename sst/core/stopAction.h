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


#ifndef _SST_STOPACTION_H
#define _SST_STOPACTION_H

#include <sst/core/action.h>

namespace SST {

class StopAction : public Action
{
public:
    StopAction() {
	setPriority(1);
    }

    void execute() {
	endSimulation();
    }
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Action);
    }
};

} // namespace SST

#endif
