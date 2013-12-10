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


#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/factory.h"

#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>

#include <ltdl.h>
#include <set>
#include <stdio.h>


#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "sst/core/component.h"
#include "sst/core/debug.h"
#include "sst/core/element.h"
#include "sst/core/introspector.h"
#include "sst/core/params.h"

/* This needs to happen before lt_dlinit() and sets up the preload
   libraries properly.  The macro declares an extern symbol, so if we
   do this in the sst namespace, the symbol is namespaced and then not
   found in linking.  So have this short function here. */
static void preload_symbols(void) {
    LTDL_SET_PRELOADED_SYMBOLS();
}

namespace SST {

/* This structure exists so that we don't need to have any
   libtool-specific code (and therefore need the libtool headers) in
   factory.h */
struct FactoryLoaderData {
    lt_dladvise advise_handle;
};

ElementLibraryInfo* followError(std::string, std::string, ElementLibraryInfo*, std::string searchPaths);

Factory::Factory(std::string searchPaths) :
    searchPaths(searchPaths)
{
    loaderData = new FactoryLoaderData;
    int ret;

    preload_symbols();

    ret = lt_dlinit();
    if (ret != 0) {
        fprintf(stderr, "lt_dlinit returned %d, %s\n", ret, lt_dlerror());
        abort();
    }

    ret = lt_dladvise_init(&loaderData->advise_handle);
    if (ret != 0) {
        fprintf(stderr, "lt_dladvise_init returned %d, %s\n", ret, lt_dlerror());
        abort();
    }

    ret = lt_dladvise_ext(&loaderData->advise_handle);
    if (ret != 0) {
        fprintf(stderr, "lt_dladvise_ext returned %d, %s\n", ret, lt_dlerror());
        abort();
    }

    ret = lt_dladvise_global(&loaderData->advise_handle);
    if (ret != 0) {
        fprintf(stderr, "lt_dladvise_global returned %d, %s\n", ret, lt_dlerror());
        abort();
    }

    ret = lt_dlsetsearchpath(searchPaths.c_str());
    if (ret != 0) {
        fprintf(stderr, "lt_dlsetsearchpath returned %d, %s\n", ret, lt_dlerror());
        abort();
    }
}


Factory::~Factory()
{
    lt_dladvise_destroy(&loaderData->advise_handle);
    lt_dlexit();

    delete loaderData;
}


Component*
Factory::CreateComponent(ComponentId_t id, 
                         std::string type, 
                         Params& params)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    // now look for component
    std::string tmp = elemlib + "." + elem;
    eic_map_t::iterator eii = 
        found_components.find(tmp);
    if (eii == found_components.end()) {
        _abort(Factory,"can't find requested component %s.\n ", tmp.c_str());
        return NULL;
    }

    const ComponentInfo ci = eii->second;

    params.pushAllowedKeys(ci.params);
    Component *ret = ci.component->alloc(id, params);
    params.popAllowedKeys();

    if (NULL == ret) return ret;

    ret->type = type;
    return ret;
}


Introspector*
Factory::CreateIntrospector(std::string type, 
                            Params& params)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    // now look for component
    std::string tmp = elemlib + "." + elem;
    eii_map_t::iterator eii = 
        found_introspectors.find(tmp);
    if (eii == found_introspectors.end()) {
        _abort(Factory,"can't find requested introspector %s.\n ", tmp.c_str());
        return NULL;
    }

    const IntrospectorInfo ii = eii->second;

    params.pushAllowedKeys(ii.params);
    Introspector *ret = ii.introspector->alloc(params);
    params.popAllowedKeys();
    return ret;
}

Module*
Factory::CreateModule(std::string type, Params& params)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    // now look for module
    std::string tmp = elemlib + "." + elem;
    eim_map_t::iterator eim = 
        found_modules.find(tmp);
    if (eim == found_modules.end()) {
        _abort(Factory,"can't find requested module %s.\n ", tmp.c_str());
        return NULL;
    }

    const ModuleInfo mi = eim->second;

    params.pushAllowedKeys(mi.params);
    Module *ret = mi.module->alloc(params);
    params.popAllowedKeys();
    return ret;
}


Module*
Factory::CreateModuleWithComponent(std::string type, Component* comp, Params& params)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(type);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    // now look for module
    std::string tmp = elemlib + "." + elem;
    eim_map_t::iterator eim = 
        found_modules.find(tmp);
    if (eim == found_modules.end()) {
        _abort(Factory,"can't find requested module %s.\n ", tmp.c_str());
        return NULL;
    }

    const ModuleInfo mi = eim->second;

    params.pushAllowedKeys(mi.params);
    Module *ret = mi.module->alloc_with_comp(comp, params);
    params.popAllowedKeys();
    return ret;
}


void
Factory::RequireEvent(std::string eventname)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(eventname);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    // initializer fires at library load time, so all we have to do is
    // make sure the event actually exists...
    if (found_events.find(eventname) == found_events.end()) {
        _abort(Factory,"can't find event %s in %s\n ", eventname.c_str(),
               searchPaths.c_str() );
    }
}

partitionFunction
Factory::GetPartitioner(std::string name)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(name);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    // Look for the partitioner
    std::string tmp = elemlib + "." + elem;
    eip_map_t::iterator eii =
	found_partitioners.find(tmp);
    if ( eii == found_partitioners.end() ) {
        _abort(Factory,"Error: Unable to find requested partitioner %s, check --help for information on partitioners.\n ", tmp.c_str());
        return NULL;
    }

    const ElementInfoPartitioner *ei = eii->second;
    return ei->func;
}

generateFunction
Factory::GetGenerator(std::string name)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(name);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    // Look for the generator
    std::string tmp = elemlib + "." + elem;
    eig_map_t::iterator eii =
	found_generators.find(tmp);
    if ( eii == found_generators.end() ) {
        _abort(Factory,"can't find requested partitioner %s.\n ", tmp.c_str());
        return NULL;
    }

    const ElementInfoGenerator *ei = eii->second;
    return ei->func;
}


genPythonModuleFunction
Factory::getPythonModule(std::string name)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(name);

    const ElementLibraryInfo *eli = findLibrary(elemlib);

    return eli->pythonModuleGenerator;
}



std::set<std::string>
Factory::create_params_set(const ElementInfoParam *params)
{
    std::set<std::string> retset;

    if (NULL != params) {
        while (NULL != params->name) {
            retset.insert(params->name);
            params++;
        }
    }

    return retset;
}


const ElementLibraryInfo*
Factory::findLibrary(std::string elemlib)
{
    const ElementLibraryInfo *eli = NULL;

    eli_map_t::iterator elii = loaded_libraries.find(elemlib);
    if (elii != loaded_libraries.end()) return elii->second;

    eli = loadLibrary(elemlib);
    if (NULL == eli) return NULL;

    loaded_libraries[elemlib] = eli;

    if (NULL != eli->components) {
        const ElementInfoComponent *eic = eli->components;
        while (NULL != eic->name) {
            std::string tmp = elemlib + "." + eic->name;
            found_components[tmp] = ComponentInfo(eic, create_params_set(eic->params));
            eic++;
        }
    }

    if (NULL != eli->events) {
        const ElementInfoEvent *eie = eli->events;
        while (NULL != eie->name) {
            std::string tmp = elemlib + "." + eie->name;
            found_events[tmp] = eie;
            if (eie->init != NULL) eie->init();
            eie++;
        }
    }

    if (NULL != eli->introspectors) {
        const ElementInfoIntrospector *eii = eli->introspectors;
        while (NULL != eii->name) {
            std::string tmp = elemlib + "." + eii->name;
            found_introspectors[tmp] = IntrospectorInfo(eii, create_params_set(eii->params));
            eii++;
        }
    }

    if (NULL != eli->modules) {
        const ElementInfoModule *eim = eli->modules;
        while (NULL != eim->name) {
            std::string tmp = elemlib + "." + eim->name;
            found_modules[tmp] = ModuleInfo(eim, create_params_set(eim->params));
            eim++;
        }
    }

    if (NULL != eli->partitioners) {
        const ElementInfoPartitioner *eip = eli->partitioners;
        while (NULL != eip->name) {
            std::string tmp = elemlib + "." + eip->name;
            found_partitioners[tmp] = eip;
            eip++;
        }
    }

    if (NULL != eli->generators) {
        const ElementInfoGenerator *eig = eli->generators;
        while (NULL != eig->name) {
            std::string tmp = elemlib + "." + eig->name;
            found_generators[tmp] = eig;
            eig++;
        }
    }

    return eli;
}


const ElementLibraryInfo*
Factory::loadLibrary(std::string elemlib)
{
    ElementLibraryInfo *eli = NULL;
    std::string libname = "lib" + elemlib;
    lt_dlhandle lt_handle;

    lt_handle = lt_dlopenadvise(libname.c_str(), loaderData->advise_handle);
    if (NULL == lt_handle) {
        // So this sucks.  the preopen module runs last and if the
        // component was found earlier, but has a missing symbol or
        // the like, we just get an amorphous "file not found" error,
        // which is totally useless...
        fprintf(stderr, "Opening element library %s failed: %s\n",
                elemlib.c_str(), lt_dlerror());
        eli = followError(libname, elemlib, eli, searchPaths);
    } else {
        // look for an info block
        std::string infoname = elemlib + "_eli";
        eli = (ElementLibraryInfo*) lt_dlsym(lt_handle, infoname.c_str());
        if (NULL == eli) {
            char *old_error = strdup(lt_dlerror());

            // backward compatibility (ugh!)  Yes, it leaks memory.  But 
            // hopefully it's going away soon.
            std::string symname = elemlib + "AllocComponent";
            void *sym = lt_dlsym(lt_handle, symname.c_str());
            if (NULL != sym) {
                eli = new ElementLibraryInfo;
                eli->name = elemlib.c_str();
                eli->description = "backward compatibility filler";
                ElementInfoComponent *elcp = new ElementInfoComponent[2];
                elcp[0].name = elemlib.c_str();
                elcp[0].description = "backward compatibility filler";
                elcp[0].printHelp = NULL;
                elcp[0].alloc = (componentAllocate) sym;
                elcp[1].name = NULL;
                elcp[1].description = NULL;
                elcp[1].printHelp = NULL;
                elcp[1].alloc = NULL;
                eli->components = elcp;
                eli->events = NULL;
                eli->introspectors = NULL;
                eli->partitioners = NULL;
                eli->generators = NULL;
                fprintf(stderr, "# WARNING: (1) Backward compatiblity initialization used to load library %s\n", elemlib.c_str());
            } else {
                fprintf(stderr, "Could not find ELI block %s in %s: %s\n",
                       infoname.c_str(), libname.c_str(), old_error);
            }

            free(old_error);
        }
    }
    return eli;
} 

std::pair<std::string, std::string>
Factory::parseLoadName(const std::string& wholename)
{
    std::size_t found = wholename.find_first_of(".");
    if (found == std::string::npos) {
        return make_pair(wholename, wholename);
    } else {
        std::string eli(wholename, 0, found);
        std::string el(wholename, (size_t)(found + 1));
        return make_pair(eli, el);
    }
}

ElementLibraryInfo*
followError(std::string libname, std::string elemlib, ElementLibraryInfo* eli, std::string searchPaths)
{

    // dlopen case
    libname.append(".so");
    std::string fullpath;
    void *handle;

    std::vector<std::string> paths;
    boost::split(paths, searchPaths, boost::is_any_of(":"));
   
    BOOST_FOREACH( std::string path, paths ) {
        struct stat sbuf;
        int ret;

        fullpath = path + "/" + libname;
        ret = stat(fullpath.c_str(), &sbuf);
        if (ret == 0) break;
    }

    // This is a little weird, but always try the last path - if we
    // didn't succeed in the stat, we'll get a file not found error
    // from dlopen, which is a useful error message for the user.
    handle = dlopen(fullpath.c_str(), RTLD_NOW|RTLD_GLOBAL);
    if (NULL == handle) {
        fprintf(stderr,
            "Opening and resolving references for element library %s failed:\n"
            "\t%s\n", elemlib.c_str(), dlerror());
        return NULL;
    }

    std::string infoname = elemlib;
    infoname.append("_eli");
    eli = (ElementLibraryInfo*) dlsym(handle, infoname.c_str());
    if (NULL == eli) {
        char *old_error = strdup(dlerror());

        // backward compatibility (ugh!)  Yes, it leaks memory.  But 
        // hopefully it's going away soon.
        std::string symname = elemlib + "AllocComponent";
        void *sym = dlsym(handle, symname.c_str());
        if (NULL != sym) {
            eli = new ElementLibraryInfo;
            eli->name = elemlib.c_str();
            eli->description = "backward compatibility filler";
            ElementInfoComponent *elcp = new ElementInfoComponent[2];
            elcp[0].name = elemlib.c_str();
            elcp[0].description = "backward compatibility filler";
            elcp[0].printHelp = NULL;
            elcp[0].alloc = (componentAllocate) sym;
            elcp[1].name = NULL;
            elcp[1].description = NULL;
            elcp[1].printHelp = NULL;
            elcp[1].alloc = NULL;
            eli->components = elcp;
            eli->events = NULL;
            eli->introspectors = NULL;
	    eli->partitioners = NULL;
	    eli->generators = NULL;
            fprintf(stderr, "# WARNING: (2) Backward compatiblity initialization used to load library %s\n", elemlib.c_str());
        } else {
            fprintf(stderr, "Could not find ELI block %s in %s: %s\n",
                    infoname.c_str(), libname.c_str(), old_error);
        }
    }


     return eli;
} 
 

} //namespace SST
