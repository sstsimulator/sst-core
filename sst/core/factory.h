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

#ifndef _SST_CORE_FACTORY_H
#define _SST_CORE_FACTORY_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <stdio.h>
#include <boost/foreach.hpp>

#include <sst/core/params.h>
#include <sst/core/elemLoader.h>
#include <sst/core/element.h>

namespace SST {

class Component;
class Introspector;
class SimulationBase;

class Factory {
public:
    Component* CreateComponent(ComponentId_t id, std::string componentname,
                               Params& params);
    Introspector* CreateIntrospector(std::string introspectorname,
                               Params& params);
    void RequireEvent(std::string eventname);

    Module* CreateModule(std::string type, Params& params);
    Module* CreateModuleWithComponent(std::string type, Component* comp, Params& params);
    
    partitionFunction GetPartitioner(std::string name);
    generateFunction GetGenerator(std::string name);
    genPythonModuleFunction getPythonModule(std::string name);
    bool hasLibrary(std::string elemlib);
private:
    friend class SST::SimulationBase;

    struct ComponentInfo {
        const ElementInfoComponent* component;
        Params::KeySet_t params;

        ComponentInfo() {}

        ComponentInfo(const ElementInfoComponent *component,
                      Params::KeySet_t params) : component(component), params(params)
        { }

        ComponentInfo(const ComponentInfo& old) : component(old.component), params(old.params)
        { }

        ComponentInfo& operator=(const ComponentInfo& old)
        {
            component = old.component;
            params = old.params;
            return *this;
        }
    };

    struct IntrospectorInfo {
        const ElementInfoIntrospector* introspector;
        Params::KeySet_t params;

        IntrospectorInfo() {}

        IntrospectorInfo(const ElementInfoIntrospector *introspector,
                         Params::KeySet_t params) : introspector(introspector), params(params)
        { }

        IntrospectorInfo(const IntrospectorInfo& old) : introspector(old.introspector), params(old.params)
        { }

        IntrospectorInfo& operator=(const IntrospectorInfo& old)
        {
            introspector = old.introspector;
            params = old.params;
            return *this;
        }
    };


    struct ModuleInfo {
        const ElementInfoModule* module;
        Params::KeySet_t params;

        ModuleInfo() {}

        ModuleInfo(const ElementInfoModule *module,
                         Params::KeySet_t params) : module(module), params(params)
        { }

        ModuleInfo(const ModuleInfo& old) : module(old.module), params(old.params)
        { }

        ModuleInfo& operator=(const ModuleInfo& old)
        {
            module = old.module;
            params = old.params;
            return *this;
        }
    };

    typedef std::map<std::string, const ElementLibraryInfo*> eli_map_t;
    typedef std::map<std::string, ComponentInfo> eic_map_t;
    typedef std::map<std::string, const ElementInfoEvent*> eie_map_t;
    typedef std::map<std::string, IntrospectorInfo> eii_map_t;
    typedef std::map<std::string, ModuleInfo> eim_map_t;
    typedef std::map<std::string, const ElementInfoPartitioner*> eip_map_t;
    typedef std::map<std::string, const ElementInfoGenerator*> eig_map_t;

    Factory(std::string searchPaths);
    ~Factory();

    Factory();                      // Don't Implement
    Factory(Factory const&);        // Don't Implement
    void operator=(Factory const&); // Don't Implement

    Params::KeySet_t create_params_set(const ElementInfoParam *params);

    // find library information for name
    const ElementLibraryInfo* findLibrary(std::string name, bool showErrors=true);
    // handle low-level loading of name
    const ElementLibraryInfo* loadLibrary(std::string name, bool showErrors=true);

    eli_map_t loaded_libraries;
    eic_map_t found_components;
    eii_map_t found_introspectors;
    eie_map_t found_events;
    eim_map_t found_modules;
    eip_map_t found_partitioners;
    eig_map_t found_generators;
    std::string searchPaths;
    ElemLoader *loader;

    std::pair<std::string, std::string> parseLoadName(const std::string& wholename);

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        std::vector<std::string> loaded_element_libraries;
        loaded_element_libraries.reserve(loaded_libraries.size());
        for (eli_map_t::const_iterator i = loaded_libraries.begin() ;
             i != loaded_libraries.end() ;
             ++i) {
            loaded_element_libraries.push_back(i->first);
        }
        ar & BOOST_SERIALIZATION_NVP(loaded_element_libraries);
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
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
};    

} // namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Factory)

#endif // SST_CORE_FACTORY_H
