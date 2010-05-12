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

#include "ptoVMapper.h"
#include "global.h" // for simAddress
#include "fe_debug.h" // for 1


//: This creates a virtual-to-physical mapping
int ptoVmapper::CreateMemRegion(const int region,
	const simAddress vaddr,
	const int len,
	const simAddress kaddr,
	const memType_t type)
{
    ptoVMapEntry entry = {region, len, kaddr, type};

    DPRINT(0,"region=%d vaddr=%#x len=%#x kaddr=%#x type =%d\n",
	    region, vaddr, len, kaddr, type);

    memMap->insert(ptoVMemMap::value_type(vaddr, entry));
    return 0;
}

//: This tests whether the address is "WC" (aka write-cached)
bool ptoVmapper::addrWC(const simAddress addr) const
{
    return (memType(addr) == WC);
}
//: This tests whether the address is "CACHED"
bool ptoVmapper::addrCached(const simAddress addr) const
{
    return (memType(addr) == CACHED);
}
//: This returns what type of address the address is
memType_t ptoVmapper::memType(const simAddress addr) const
{
    if (memMap->empty()) {
	return CACHED;
    }

    ptoVMemMap::const_iterator iter = memMap->upper_bound(addr);
    if (iter == memMap->begin()) {
	return CACHED;
    }
    --iter;

    ptoVMapEntry entry = (*iter).second;
    simAddress offset = addr - (*iter).first;

    if (offset < (simAddress)entry.len) {
	DPRINT(1,"found map for addr=%#x %#x %#x %d %#lx\n",
		addr, (*iter).first, entry.kaddr, entry.len, 
        (unsigned long) entry.kaddr+offset);
	return entry.type;
    } else if (entry.region >= 100) {
	ERROR("invalid address=%#x\n", addr);
    }
    return CACHED;
}
//: This transforms a virtual address into a physical address
simAddress ptoVmapper::getPhysAddr(const simAddress addr) const
{
    if (memMap->empty()) {
	return addr;
    }

    ptoVMemMap::const_iterator iter = memMap->upper_bound(addr);
    if (iter == memMap->begin()) {
	return addr;
    }
    --iter;

    ptoVMapEntry entry = (*iter).second;
    simAddress offset = addr - (*iter).first;

    if (offset < (simAddress)entry.len) {
	DPRINT(1,"found map for addr=%#x %#x %#x %d %#lx\n",
		addr, (*iter).first, entry.kaddr, entry.len, 
        (unsigned long) entry.kaddr+offset);
	return entry.kaddr+offset;
    } else if (entry.region >= 100) {
	ERROR("invalid address=%#x\n", addr);
    }
    return addr;
}
