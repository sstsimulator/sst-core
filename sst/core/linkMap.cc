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


#include<sst_config.h>

#include <sst/core/link.h>
#include <sst/core/component.h>
#include <sst/core/linkMap.h>
#include <sst/core/simulation.h>

namespace SST {

LinkMap::LinkMap( ComponentId_t myId ) :
    myId( myId )
{
    _LM_DBG( "\n" );
}

Link* LinkMap::LinkAdd( std::string name, Event::Handler_t* functor )
{
    _LM_DBG( "name=%s functor=%p\n", name.c_str(), functor );

    if ( linkMap.count( name ) ) {
	_LM_DBG( "BAD: linkMap.count is %ld for link \"%s\"\n", (long)linkMap.count( name ), name.c_str() );
        return NULL;
    }

    Link *link = new Link( functor );
    if ( link == NULL )   {
	_LM_DBG( "BAD: Out of memory\n");
        return NULL;
    }

    return LinkAdd( name, link );
}
    
Link* LinkMap::LinkAdd( std::string name, Link* link )
{
    _LM_DBG( "name=%s link=%p\n", name.c_str(), link );

    if ( linkMap.count( name ) ) {
	_LM_DBG( "BAD: linkMap.count is %ld for link \"%s\"\n", (long)linkMap.count( name ), name.c_str() );
        return NULL;
    }

    linkMap[ name ] = link; 

    return link;
}

Link* LinkMap::selfLink( std::string name, Event::Handler_t* handler )
{
    Link *lnk = LinkAdd(name, handler);
    // Don't know if this will work, but simply try connecting the
    // link to itself
    lnk->Connect(lnk,0);
    return lnk;
}

Link* LinkMap::LinkGet( std::string name )
{
    _LM_DBG( "name=%s\n", name.c_str() );

    if ( ! linkMap.count( name ) ) {
	_LM_DBG( "BAD: linkMap.count is %ld for link \"%s\"\n", (long)linkMap.count( name ), name.c_str() );
        return NULL;
    }

    return linkMap[ name ];
}

int LinkMap::LinkConnect( std::string name, Link* dest, Cycle_t lat )
{
    _LM_DBG( "link=%s dest=%p lat=%lu\n", name.c_str(), dest, (unsigned long) lat );
    Link *src = LinkGet( name );

    if ( ! src  ) {
	_LM_DBG( "Could not get link for \"%s\"\n", name.c_str());
        return -1;
    }

    _LM_DBG( "src=%p dest=%p\n", src, dest );
    src->Connect( dest, lat );
    return 0;
}

} // namespace SST
