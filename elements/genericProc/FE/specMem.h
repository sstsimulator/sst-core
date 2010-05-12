// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2007, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SPECMEM_H
#define SPECMEM_H

#include "global.h"
#include <map>

using namespace std;

class memory_interface;

//: Speculative memory implementation
//
// An object which contains data which is speculative and designed to
// be dumped.
//
//!SEC:Framework
class specMemory {
  typedef map<simAddress, uint8> specMap;
  map<simAddress, uint8> specData;
  memory_interface *mem;

  uint8 getSpecByte(const simAddress sa);

  void writeSpecByte(const simAddress sa, const uint8 Data) {
    specData[sa] = Data;
  }

  bool useSpec(const simAddress sa, specMap::iterator &i) {
    i = specData.find(sa);
    if (i != specData.end()) {
      return 1;
    } else {
      return 0;
    }
  }

public:
  specMemory(memory_interface *m) : mem(m) {;}

  void squashSpec() {
    specData.clear();
  }

  uint8 readSpec8(const simAddress sa) {
    return getSpecByte(sa);
  }

  uint16 readSpec16(const simAddress sa) {
    uint16 r = (getSpecByte(sa) << 8);
    r += getSpecByte(sa+1);
    return r;
  }

  uint32 readSpec32(const simAddress sa) {
    uint32 r = (getSpecByte(sa) << 24);
    r += getSpecByte(sa+1) << 16;
    r += getSpecByte(sa+2) << 8;
    r += getSpecByte(sa+3);
    return r;
  }

  bool writeSpec8(const simAddress sa, const uint8 Data) {
    writeSpecByte(sa, Data);
    return 1;
  }

  bool writeSpec16(const simAddress sa, const uint16 Data) {
    writeSpecByte(sa, Data>>8);
    writeSpecByte(sa+1, Data & 0xff);
    return 1;
  }

  bool writeSpec32(const simAddress sa, const uint32 Data) {
    writeSpecByte(sa, Data>>24);
    writeSpecByte(sa+1, Data>>16 & 0xff);
    writeSpecByte(sa+2, Data>>8 & 0xff);
    writeSpecByte(sa+3, Data & 0xff);
    return 1;
  }
};

#endif
