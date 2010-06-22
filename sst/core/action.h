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


#ifndef SST_ACTION_H
#define SST_ACTION_H

#include <sst/core/activity.h>

namespace SST {

class Action : public Activity {
public:
    Action() {}
    ~Action() {}

protected:
    void endSimulation();
    
private:
     friend class boost::serialization::access;
     template<class Archive>
     void
     serialize(Archive & ar, const unsigned int version )
     {
         boost::serialization::base_object<Activity>(*this);
     }
};

}

#endif // SST_ACTION_H
