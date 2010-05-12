// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



#ifndef LOADINFO_H
#define LOADINFO_H

#include <global.h>
#include "sst/boost.h"

struct LoadInfo { 
  friend class boost::serialization::access;
  template<class Archive> 
  void serialize(Archive & ar, const unsigned int version )
  {
    ar & BOOST_SERIALIZATION_NVP(constrLoc);
    ar & BOOST_SERIALIZATION_NVP(constrSize);
    ar & BOOST_SERIALIZATION_NVP(start_addr);
    ar & BOOST_SERIALIZATION_NVP(stack_base);   
    ar & BOOST_SERIALIZATION_NVP(text_addr);
    ar & BOOST_SERIALIZATION_NVP(text_len);
    ar & BOOST_SERIALIZATION_NVP(data_addr);
    ar & BOOST_SERIALIZATION_NVP(data_len);
    ar & BOOST_SERIALIZATION_NVP(heap_addr);
    ar & BOOST_SERIALIZATION_NVP(heap_len);
    ar & BOOST_SERIALIZATION_NVP(stack_addr);
    ar & BOOST_SERIALIZATION_NVP(stack_len);
  }

  //: location of constructor section of the header
  simAddress constrLoc;
  //: length of constructor section
  simAddress constrSize;

  simAddress start_addr;
  simAddress stack_base;

  simAddress text_addr;
  unsigned long text_len;

  simAddress data_addr;
  unsigned long data_len;

  simAddress heap_addr;
  unsigned long heap_len;

  simAddress stack_addr;
  unsigned long stack_len;
};
#endif
