// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_LINKPAIR_H
#define SST_CORE_LINKPAIR_H

#include <sst/core/sst_types.h>

#include <sst/core/link.h>

namespace SST {

/**
 * Defines a pair of links (to define a connected link)
 */
class LinkPair {
public:
    /** Create a new LinkPair with specified ID */
    LinkPair(LinkId_t id) :
        left(new Link(id)),
        right(new Link(id))
    {
        my_id = id;

        left->pair_link = right;
        right->pair_link = left;

    }
    virtual ~LinkPair() {}

    /** return the ID of the LinkPair */
    LinkId_t getId() {
        return my_id;
    }

    /** Return the Left Link */
    inline Link* getLeft() {return left;}
    /** Return the Right Link */
    inline Link* getRight() {return right;}

private:

    Link* left;
    Link* right;

    LinkId_t my_id;

};

} //namespace SST

#endif // SST_CORE_LINKPAIR_H
