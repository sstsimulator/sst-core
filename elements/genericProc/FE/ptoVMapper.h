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

#ifndef SST_PTOVMAPPER_H
#define SST_PTOVMAPPER_H

#include "global.h" // for simAddress
#include <map>
#include "sst/boost.h"

typedef enum { CACHED, UNCACHED, WC } memType_t;

struct ptoVMapEntry {
#if WANT_CHECKPOINT_SUPPORT
  BOOST_SERIALIZE {
    ar & BOOST_SERIALIZATION_NVP(region);
    ar & BOOST_SERIALIZATION_NVP(len);
    ar & BOOST_SERIALIZATION_NVP(kaddr);
    ar & BOOST_SERIALIZATION_NVP(type);
  }
#endif
    int region;
    int len;
    simAddress kaddr;
    memType_t type;
};

typedef std::map<simAddress,ptoVMapEntry> ptoVMemMap;

class ptoVmapper
{
#if WANT_CHECKPOINT_SUPPORT
  BOOST_SERIALIZE {
    ar & BOOST_SERIALIZATION_NVP(memMap);    
  }
#endif
    ptoVMemMap *memMap;

    public:
    ptoVmapper() : memMap(new ptoVMemMap) {}
    ptoVmapper(ptoVMemMap *p) : memMap(p) {}
    ptoVmapper(ptoVmapper *p) : memMap(p->memMap) {}

    virtual ~ptoVmapper() {;}
    //: This creates a virtual-to-physical mapping
    virtual int CreateMemRegion(const int region,
	    const simAddress vaddr,
	    const int len,
	    const simAddress kaddr,
	    const memType_t type = CACHED);

    //: This tests whether the address is "WC" (aka write-cached)
    virtual bool addrWC(const simAddress addr) const;
    //: This tests whether the address is "CACHED"
    virtual bool addrCached(const simAddress addr) const;
    //: This returns what type of address the address is
    virtual memType_t memType(const simAddress addr) const;
    //: This transforms a virtual address into a physical address
    virtual simAddress getPhysAddr(const simAddress addr) const;
};

#endif
