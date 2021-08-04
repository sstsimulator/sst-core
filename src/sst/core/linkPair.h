// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
    /** Create a new LinkPair with specified ID */
#if !SST_BUILDING_CORE
    LinkPair(LinkId_t id) __attribute__((
        deprecated("LinkPair class was not intended to be used outside of SST Core and will be removed in SST 12."))) :
#else
    LinkPair(LinkId_t id) :
#endif
        left(new Link(id)),
        right(new Link(id))
    {
        my_id = id;

        left->pair_link  = right;
        right->pair_link = left;
    }

#if !SST_BUILDING_CORE
    virtual ~LinkPair() __attribute__((
        deprecated("LinkPair class was not intended to be used outside of SST Core and will be removed in SST 12.")))
    {}
#else
    virtual ~LinkPair() {}
#endif

    /** return the ID of the LinkPair */
#if !SST_BUILDING_CORE
    LinkId_t getId() __attribute__((
        deprecated("LinkPair class was not intended to be used outside of SST Core and will be removed in SST 12.")))
    {
#else
    LinkId_t getId()
    {
#endif
        return my_id;
    }

    /** Return the Left Link */
#if !SST_BUILDING_CORE
    inline Link* getLeft() __attribute__((
        deprecated("LinkPair class was not intended to be used outside of SST Core and will be removed in SST 12.")))
    {
        return left;
    }
#else
    inline Link* getLeft() { return left; }
#endif
    /** Return the Right Link */
#if !SST_BUILDING_CORE
    inline Link* getRight() __attribute__((
        deprecated("LinkPair class was not intended to be used outside of SST Core and will be removed in SST 12.")))
    {
        return right;
    }
#else
    inline Link* getRight() { return right; }
#endif

private:
    Link* left;
    Link* right;

    LinkId_t my_id;
};

} // namespace SST

#endif // SST_CORE_LINKPAIR_H
