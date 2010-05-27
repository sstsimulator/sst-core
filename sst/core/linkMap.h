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


#ifndef SST_LINKMAP_H
#define SST_LINKMAP_H

#include <vector>

#include <sst/core/sst.h>
#include <sst/core/event.h>
#include <sst/core/eventQueue.h>

namespace SST { 

#define _LM_DBG( fmt, args...) __DBG( DBG_LINKMAP, LinkMap, fmt, ## args )

class Link;
 
  /** Structure for tracking and connecting Links */
class LinkMap {
    protected:
        typedef std::map< std::string, Link* >  linkMap_t;

    public:
        LinkMap( ComponentId_t id = -1 );

  /** Create a Link object, assigning a name and event handler.
      Returns the link object, or NULL if a link by that name already
      exists.
      
      @param name Name of the link. This name should match the <name>
      of the link for that component in the XML file.

      @param handler The event handler to handle events from this
      link. If NULL, the link must be polled for incoming events. 
   */
        Link* LinkAdd( std::string name, Event::Handler_t* handler = NULL );
        Link* LinkAdd( std::string, Link* );

  /** Create a link to yourself.
      
      @param name Name for the Link
      @param handler The event handler to handle events from this
      link. If NULL, the link must be polled for incoming events.
   */
	Link* selfLink( std::string name, Event::Handler_t* handler = NULL );
	
  /** Return the Link by name.
   @params name name of the link to be returned */
        Link* LinkGet( std::string name );
  /** Return the nth link to this component */
//         Link* LinkGet( unsigned int num );

        int LinkConnect( std::string str_name, Link* link, Cycle_t lat ); 
	
    protected:
  /** Unique component ID */
        ComponentId_t   myId;

        linkMap_t       linkMap;

    private:

        LinkMap( const LinkMap& l );

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version )
	{
	    ar & BOOST_SERIALIZATION_NVP( myId );
	    ar & BOOST_SERIALIZATION_NVP( linkMap );
	}
};

} // namespace SST

#endif // SST_LINKMAP_H
