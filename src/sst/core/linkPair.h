// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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
    /** Create a new LinkPair.  This is used when the endpoints are in the same partition.

        @param id ID of the link
    */
    explicit LinkPair(LinkId_t id) :
        left(new Link(id)),
        right(new Link(id))
    {
        left->pair_link  = right;
        right->pair_link = left;
    }

    /** Create a new LinkPair.  This is used on restart.

        @param order Value used to enforce the link order.
    */
    LinkPair() :
        left(new Link()),
        right(new Link())
    {
        left->pair_link  = right;
        right->pair_link = left;
    }


    virtual ~LinkPair() {}

    /** Return the Left Link
     * @return Left link
     */
    inline Link* getLeft() { return left; }

    /** Return the Right Link
     * @return Right link
     */
    inline Link* getRight() { return right; }

private:
    Link* left;
    Link* right;
};

} // namespace SST

#endif // SST_CORE_LINKPAIR_H
