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


#ifndef _SST_FACTORY_H
#define _SST_FACTORY_H

#include <boost/foreach.hpp>

#include <sst/core/sst.h>
#include <sst/core/component.h>
#include <sst/core/introspector.h>
#include <sst/core/element.h>

namespace SST {

class SimulationBase;
struct FactoryLoaderData;

class Factory {
public:
    Component* CreateComponent(ComponentId_t id, std::string componentname,
                               Component::Params_t& params);
    void RegisterEvent(std::string eventname);
  
private:
    friend class SST::SimulationBase;

    typedef std::map<std::string, const ElementLibraryInfo*> eli_map_t;
    typedef std::map<std::string, const ElementInfoComponent*> eic_map_t;
    typedef std::map<std::string, const ElementInfoEvent*> eie_map_t;

    Factory(std::string searchPaths);
    ~Factory();

    Factory();                      // Don't Implement
    Factory(Factory const&);        // Don't Implement
    void operator=(Factory const&); // Don't Implement

    // find library information for name
    const ElementLibraryInfo* findLibrary(std::string name);
    // handle low-level loading of name
    const ElementLibraryInfo* loadLibrary(std::string name);

    eli_map_t loaded_libraries;
    eic_map_t found_components;
    eie_map_t found_events;
    std::string searchPaths;
    FactoryLoaderData *loaderData;

    std::pair<std::string, std::string> parseLoadName(std::string wholename);

#if WANT_CHECKPOINT_SUPPORT

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        std::vector<std::string> loaded_components;
        loaded_components.resize(allocMap.size());

        for (allocMap_t::const_iterator i = allocMap.begin() ;
             i != allocMap.end() ;
             ++i) {
            loaded_components.push_back(i->first);
        }
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        std::vector<std::string> loaded_components;

        ar >> BOOST_SERIALIZATION_NVP( loaded_components ); 

        BOOST_FOREACH( std::string type, loaded_components ) {
            if ( allocMap.find( type ) == allocMap.end() ) { 
                allocMap[ type ] = libFind( type );
            }
        }
    }

    template<class Archive>
    friend void save_construct_data(Archive & ar, 
                                    const Factory * t, 
                                    const unsigned int file_version)
    {
        std::string search_path = t->searchPaths;

        ar << BOOST_SERIALIZATION_NVP(search_path);
    }

    template<class Archive>
    friend void load_construct_data(Archive & ar, 
                                    Factory * t,
                                    const unsigned int file_version)
    {
        std::string search_path;
        ar >> BOOST_SERIALIZATION_NVP(search_path);
        ::new(t)Factory(search_path);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

#endif
	
};    

} // namespace SST

#endif
