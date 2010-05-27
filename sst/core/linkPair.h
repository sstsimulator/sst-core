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


#ifndef SST_LINKPAIR_H
#define SST_LINKPAIR_H

#include <sst/core/sst.h>

namespace SST {

class LinkPair {
public:
    LinkPair() {
	// Just create the two links and hook them together
	left = new Link();
	right = new Link();

	left->pair_link = right;
	right->pair_link = left;
    }
    virtual LinkPair() {}

private:
    
    Link *left;
    Link *right;
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
    }
};

}

#endif // SST_LINKPAIR_H
