// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_MODULE_H
#define SST_CORE_MODULE_H

namespace SST {
  /**
     Module is a tag class used with the loadModule function.
   */
    class Module {

    public:
	Module() {}
	virtual ~Module() {}

//  private:
//      friend class boost::serialization::access;
//      template<class Archive>
//      void
//      serialize(Archive & ar, const unsigned int version )
//      {
//      }
    };
} //namespace SST

// BOOST_CLASS_EXPORT_KEY(SST::Module)

#endif // SST_CORE_ACTION_H
