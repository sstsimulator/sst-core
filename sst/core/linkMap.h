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

#include <string>
#include <map>

#include <sst/core/sst_types.h>
#include <sst/core/link.h>

namespace SST { 

#define _LM_DBG( fmt, args...) __DBG( DBG_LINKMAP, LinkMap, fmt, ## args )

class Link;

class LinkMap {

private:
    std::map<std::string,Link*> linkMap;

public:
    LinkMap() {}
    ~LinkMap() {}
    
    void insertLink(std::string name, Link* link) {
	linkMap.insert(std::pair<std::string,Link*>(name,link));
    }

    Link* getLink(std::string name) {
	std::map<std::string,Link*>::iterator it = linkMap.find(name);
	if ( it == linkMap.end() ) return NULL;
	else return it->second;
    }

    // FIXME: Cludge for now
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

#endif // SST_LINKMAP_H
