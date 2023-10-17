// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_LINKPAIR_H
#define SST_CORE_LINKPAIR_H

#include "sst/core/link.h"
#include "sst/core/sst_types.h"

namespace SST {

/**
 * Defines a pair of links (to define a connected link)
 */
class LinkPair
{
public:
    /** Create a new LinkPair.  This is used when the endpoints are in the same partition. */
    LinkPair(LinkId_t order) : left(new Link(order)), right(new Link(order))
    {
        my_id = order;

        left->pair_link  = right;
        right->pair_link = left;
    }

    /** Create a new LinkPair.  This is used when the endpoints are in different partitions. */
    LinkPair(LinkId_t order, LinkId_t remote_tag) : left(new Link(remote_tag)), right(new Link(order))
    {
        my_id = order;

        left->pair_link  = right;
        right->pair_link = left;
    }

    virtual ~LinkPair() {}

    /** Return the Left Link */
    inline Link* getLeft() { return left; }

    /** Return the Right Link */
    inline Link* getRight() { return right; }

private:
    Link* left;
    Link* right;

    LinkId_t my_id;
};

} // namespace SST

#endif // SST_CORE_LINKPAIR_H
