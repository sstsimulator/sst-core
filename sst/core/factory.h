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

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/introspector.h>
#include <sst/core/element.h>

namespace SST {

class SimulationBase;
struct FactoryLoaderData;

class Factory {
public:
    Component* CreateComponent(ComponentId_t id, std::string componentname,
                               Params& params);
    Introspector* CreateIntrospector(std::string introspectorname,
                               Params& params);
    void RequireEvent(std::string eventname);

    partitionFunction GetPartitioner(std::string name);
    generateFunction GetGenerator(std::string name);
private:
    friend class SST::SimulationBase;

    typedef std::map<std::string, const ElementLibraryInfo*> eli_map_t;
    typedef std::map<std::string, const ElementInfoComponent*> eic_map_t;
    typedef std::map<std::string, const ElementInfoEvent*> eie_map_t;
    typedef std::map<std::string, const ElementInfoIntrospector*> eii_map_t;
    typedef std::map<std::string, const ElementInfoPartitioner*> eip_map_t;
    typedef std::map<std::string, const ElementInfoGenerator*> eig_map_t;

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
    eii_map_t found_introspectors;
    eie_map_t found_events;
    eip_map_t found_partitioners;
    eig_map_t found_generators;
    std::string searchPaths;
    FactoryLoaderData *loaderData;

    std::pair<std::string, std::string> parseLoadName(std::string wholename);

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        printf("begin Factory::save\n");
        std::vector<std::string> loaded_element_libraries;
        loaded_element_libraries.reserve(loaded_libraries.size());
        for (eli_map_t::const_iterator i = loaded_libraries.begin() ;
             i != loaded_libraries.end() ;
             ++i) {
            loaded_element_libraries.push_back(i->first);
        }
        printf("end Factory::save (%d)\n", (int) loaded_element_libraries.size());
        ar & BOOST_SERIALIZATION_NVP(loaded_element_libraries);
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        printf("begin Factory::load\n");
        std::vector<std::string> loaded_element_libraries;
        ar & BOOST_SERIALIZATION_NVP(loaded_element_libraries); 
        BOOST_FOREACH(std::string type, loaded_element_libraries) {
            if (NULL == findLibrary(type)) {
                fprintf(stderr, 
                        "factory::load() failed to load %s\n", 
                        type.c_str());
                abort();
            }
        }
        printf("end Factory::load (%d)\n", (int) loaded_element_libraries.size());
    }

    template<class Archive>
    friend void save_construct_data(Archive & ar, 
                                    const Factory * t, 
                                    const unsigned int file_version)
    {
        printf("begin Factory::save_construct_data\n");
        std::string search_path = t->searchPaths;
        ar << BOOST_SERIALIZATION_NVP(search_path);
        printf("end Factory::save_construct_data\n");
    }

    template<class Archive>
    friend void load_construct_data(Archive & ar, 
                                    Factory * t,
                                    const unsigned int file_version)
    {
        printf("begin Factory::load_construct_data\n");
        std::string search_path;
        ar >> BOOST_SERIALIZATION_NVP(search_path);
        ::new(t)Factory(search_path);
        printf("end Factory::load_construct_data\n");
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};    

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Factory)

#endif
