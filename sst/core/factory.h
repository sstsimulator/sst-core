// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
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

/**
 * Class for instantiating Components, Links and the like out
 * of element libraries.
 */
class Factory {
public:
    /** Attempt to create a new Component instantiation
     * @param id - The unique ID of the component instantiation
     * @param componentname - The fully qualified elementlibname.componentname type of component
     * @param params - The params to pass to the component's constructor
     * @return Newly created component
     */
    Component* CreateComponent(ComponentId_t id, std::string componentname,
                               Params& params);
    /** Attempt to create a new Introspector instantiation
     * @param introspectorname - The fully qualified elementlibname.introspectorname type of introspector
     * @param params - The params to pass to the introspectors's constructor
     * @return Newly created introspector
     */
    Introspector* CreateIntrospector(std::string introspectorname,
                               Params& params);

    /** Ensure that an element library containing the required event is loaded
     * @param eventname - The fully qualified elementlibname.eventname type
     */
    void RequireEvent(std::string eventname);

    /** Instatiate a new Module
     * @param type - Fully qualified elementlibname.modulename type
     * @param params - Parameters to pass to the Module's constructor
     */
    Module* CreateModule(std::string type, Params& params);

    /** Instatiate a new Module
     * @param type - Fully qualified elementlibname.modulename type
     * @param comp - Component instance to pass to the Module's constructor
     * @param params - Parameters to pass to the Module's constructor
     */
    Module* CreateModuleWithComponent(std::string type, Component* comp, Params& params);

    /** Return partitioner function
     * @param name - Fully qualified elementlibname.partitioner type name
     */
    partitionFunction GetPartitioner(std::string name);
    /** Return generator function
     * @param name - Fully qualified elementlibname.generator type name
     */
    generateFunction GetGenerator(std::string name);
    /** Return Python Module creation function
     * @param name - Fully qualified elementlibname.pythonModName type name
     */
    genPythonModuleFunction getPythonModule(std::string name);
    /** Checks to see if library exists and can be loaded */
    bool hasLibrary(std::string elemlib);

    void getLoadedLibraryNames(std::set<std::string>& lib_names);
    void loadUnloadedLibraries(const std::set<std::string>& lib_names);
    
private:
    friend class SST::SimulationBase;

    struct ComponentInfo {
        const ElementInfoComponent* component;
        Params::KeySet_t params;
        std::vector<std::string> ports;

        ComponentInfo() {}

        ComponentInfo(const ElementInfoComponent *component, Params::KeySet_t params) :
            component(component), params(params)
        {
            const ElementInfoPort *p = component->ports;
            while ( NULL != p && NULL != p->name ) {
                ports.push_back(p->name);
                p++;
            }
        }

        ComponentInfo(const ComponentInfo& old) : component(old.component), params(old.params), ports(old.ports)
        { }

        ComponentInfo& operator=(const ComponentInfo& old)
        {
            component = old.component;
            params = old.params;
            ports = old.ports;
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
