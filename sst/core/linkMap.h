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


#ifndef SST_LINKMAP_H
#define SST_LINKMAP_H

#include <string>
#include <map>

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

namespace SST { 

#define _LM_DBG( fmt, args...) __DBG( DBG_LINKMAP, LinkMap, fmt, ## args )

class Link;

class LinkMap {

private:
    std::map<std::string,Link*> linkMap;

public:
    LinkMap() {}
    ~LinkMap() {
	// Delete all the links in the map
	for ( std::map<std::string,Link*>::iterator it = linkMap.begin(); it != linkMap.end(); ++it ) {
	    delete it->second;
	}
	linkMap.clear();
    }
    
    void insertLink(std::string name, Link* link) {
	linkMap.insert(std::pair<std::string,Link*>(name,link));
    }

    Link* getLink(std::string name) {
	std::map<std::string,Link*>::iterator it = linkMap.find(name);
	if ( it == linkMap.end() ) return NULL;
	else return it->second;
    }

    // FIXME: Cludge for now, fix later.  Need to make LinkMap look
    // like a regular map instead.
    std::map<std::string,Link*>& getLinkMap() {
	return linkMap;
    }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_NVP(linkMap);
    }
};

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::LinkMap)

#endif // SST_LINKMAP_H
