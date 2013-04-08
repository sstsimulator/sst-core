// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_LINKPAIR_H
#define SST_LINKPAIR_H

#include <sst/core/sst_types.h>
#include <sst/core/link.h>

namespace SST {

class LinkPair {
public:
    LinkPair(LinkId_t id) :
	left(new Link(id)),
	right(new Link(id))
    {
	my_id = id;

	left->pair_link = right;
	right->pair_link = left;

    }
    virtual ~LinkPair() {}

    LinkId_t getId() {
	return my_id;
    }

    inline Link* getLeft() {return left;}
    inline Link* getRight() {return right;}
    
private:
    
    Link* left;
    Link* right;

    LinkId_t my_id;
    
    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
    }
};

}

BOOST_CLASS_EXPORT_KEY(SST::LinkPair)

#endif // SST_LINKPAIR_H
