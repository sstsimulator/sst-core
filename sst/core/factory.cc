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


#include "sst_config.h"
#include "sst/core/serialization/core.h"

#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>

#include <stdio.h>

#if SST_HAVE_LIBLTDL_SUPPORT
#include <ltdl.h>
#endif

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

#include "sst/core/factory.h"
#include "sst/core/element.h"
#include "sst/core/params.h"

namespace SST {

/* This structure exists so that we don't need to have any
   libtool-specific code (and therefore need the libtool headers) in
   factory.h */
struct FactoryLoaderData {
#ifdef HAVE_LT_DLADVISE
    lt_dladvise advise_handle;
#endif
};

Factory::Factory(std::string searchPaths) :
    searchPaths(searchPaths)
{
    loaderData = new FactoryLoaderData;

#if SST_HAVE_LIBLTDL_SUPPORT
    int ret;

    ret = lt_dlinit();
    if (ret != 0) {
        fprintf(stderr, "lt_dlinit returned %d, %s\n", ret, lt_dlerror());
        abort();
    }

#ifdef HAVE_LT_DLADVISE
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
#endif

    ret = lt_dlsetsearchpath(searchPaths.c_str());
    if (ret != 0) {
        fprintf(stderr, "lt_dlsetsearchpath returned %d, %s\n", ret, lt_dlerror());
        abort();
    }
#endif
}


Factory::~Factory()
{
#if SST_HAVE_LIBLTDL_SUPPORT
#ifdef HAVE_LT_DLADVISE
    lt_dladvise_destroy(&loaderData->advise_handle);
#endif
    lt_dlexit();
#endif

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

    const ElementInfoComponent *ei = eii->second;
    Component *ret = ei->alloc(id, params);
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

    const ElementInfoIntrospector *ei = eii->second;
    Introspector *ret = ei->alloc(params);
    return ret;
}


void
Factory::RegisterEvent(std::string eventname)
{
    std::string elemlib, elem;
    boost::tie(elemlib, elem) = parseLoadName(eventname);

    // ensure library is already loaded...
    if (loaded_libraries.find(elemlib) == loaded_libraries.end()) {
        findLibrary(elemlib);
    }

    // initializer fires at library load time, so all we have to do is
    // make sure the event actually exists...
    if (found_events.find(elem) == found_events.end()) {
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
        _abort(Factory,"can't find requested partitioner %s.\n ", tmp.c_str());
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
            found_components[tmp] = eic;
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
            found_introspectors[tmp] = eii;
            eii++;
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

#if SST_HAVE_LIBLTDL_SUPPORT
    lt_dlhandle lt_handle;

#ifdef HAVE_LT_DLADVISE
    lt_handle = lt_dlopenadvise(libname.c_str(), loaderData->advise_handle);
#else
    lt_handle = lt_dlopenext(libname.c_str());
#endif
    if (NULL == lt_handle) {
        // So this sucks.  the preopen module runs last and if the
        // component was found earlier, but has a missing symbol or
        // the like, we just get an amorphous "file not found" error,
        // which is totally useless, so we instead fall to the dlopen
        // case in hopes it can be more useful...
        const char *tmp = lt_dlerror();
        if (0 != strcmp(tmp, "file not found")) {
            fprintf(stderr, "Opening element library %s failed: %s\n",
                    elemlib.c_str(), tmp);
            return NULL;
        }
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
        return eli;
    }
#endif // SST_HAVE_LIBLTDL_SUPPORT

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
        fprintf(stderr, "Opening element library %s failed: %s\n",
               elemlib.c_str(), dlerror());
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

std::pair<std::string, std::string>
Factory::parseLoadName(std::string wholename)
{
    std::vector<std::string> parts;
    boost::split(parts, wholename, boost::is_any_of("."));
    if (parts.size() == 1) {
        return make_pair(parts[0], parts[0]);
    } else {
        return make_pair(parts[0], parts[1]);
    }
}

} //namespace SST
