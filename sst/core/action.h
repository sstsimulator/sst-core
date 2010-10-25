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
    virtual ~Action() {}

    void print(const std::string& header) const {
        std::cout << header << "Generic Action to be delivered at "
                  << getDeliveryTime() << " with priority " << getPriority() << std::endl;
    }

    
protected:
    void endSimulation();
    
private:
     friend class boost::serialization::access;
     template<class Archive>
     void
     serialize(Archive & ar, const unsigned int version )
     {
         ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Activity);
     }
};

}

BOOST_CLASS_EXPORT_KEY(SST::Action)

#endif // SST_ACTION_H
