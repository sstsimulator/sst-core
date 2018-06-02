// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include "elemLoader.h"

#include <ltdl.h>
#include <vector>

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

#include <cstring>

#include <stdio.h>

/* This needs to happen before lt_dlinit() and sets up the preload
   libraries properly.  The macro declares an extern symbol, so if we
   do this in the sst namespace, the symbol is namespaced and then not
   found in linking.  So have this short function here.

   Only do this when building with element libraries.
   */
static void preload_symbols(void) {
// README: This is only set if we are not building any elements
// in the split up build this is now default so must always be called.
//#ifndef __SST_BUILD_CORE_ONLY__
//    LTDL_SET_PRELOADED_SYMBOLS();
//#endif
}

namespace SST {

/** This structure exists so that we don't need to have any
   libtool-specific code (and therefore need the libtool headers) in
   factory.h */
struct LoaderData {
    /** Handle from Libtool */
    lt_dladvise advise_handle;
};


ElemLoader::ElemLoader(const std::string &searchPaths) :
    searchPaths(searchPaths)
{
    loaderData = new LoaderData;
    int ret = 0;

    preload_symbols();


    ret = lt_dlinit();
    if (ret != 0) {
        fprintf(stderr, "lt_dlinit returned %d, %s\n", ret, lt_dlerror());
        delete loaderData;
        abort();
    }

    ret = lt_dladvise_init(&loaderData->advise_handle);
    if (ret != 0) {
        fprintf(stderr, "lt_dladvise_init returned %d, %s\n", ret, lt_dlerror());
        delete loaderData;
        abort();
    }

    ret = lt_dladvise_ext(&loaderData->advise_handle);
    if (ret != 0) {
        fprintf(stderr, "lt_dladvise_ext returned %d, %s\n", ret, lt_dlerror());
        delete loaderData;
        abort();
    }

    ret = lt_dladvise_global(&loaderData->advise_handle);
    if (ret != 0) {
        fprintf(stderr, "lt_dladvise_global returned %d, %s\n", ret, lt_dlerror());
        delete loaderData;
        abort();
    }

    ret = lt_dlsetsearchpath(searchPaths.c_str());
    if (ret != 0) {
        fprintf(stderr, "lt_dlsetsearchpath returned %d, %s\n", ret, lt_dlerror());
        delete loaderData;
        abort();
    }

}


ElemLoader::~ElemLoader()
{
    if ( loaderData ) {
        lt_dladvise_destroy(&loaderData->advise_handle);
        lt_dlexit();

        delete loaderData;
    }
}


static std::vector<std::string> splitPath(const std::string & searchPaths)
{
    std::vector<std::string> paths;
    char * pathCopy = new char [searchPaths.length() + 1];
    std::strcpy(pathCopy, searchPaths.c_str());
    char *brkb = NULL;
    char *p = NULL;
    for ( p = strtok_r(pathCopy, ":", &brkb); p ; p = strtok_r(NULL, ":", &brkb) ) {
        paths.push_back(p);
    }

    delete [] pathCopy;
    return paths;
}


static ElementLibraryInfo* followError(std::string libname, std::string elemlib, ElementLibraryInfo* eli, std::string searchPaths)
{

    // dlopen case
    libname.append(".so");
    std::string fullpath;
    void *handle;

    std::vector<std::string> paths = splitPath(searchPaths);

    for ( std::string path : paths ) {
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

        fprintf(stderr, "Could not find ELI block %s in %s: %s\n",
                infoname.c_str(), libname.c_str(), old_error);
    }


     return eli;
}

const ElementLibraryInfo*
ElemLoader::loadLibrary(const std::string &elemlib, bool showErrors)
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
        if (showErrors) {
            fprintf(stderr, "Opening element library %s failed: %s\n",
                    elemlib.c_str(), lt_dlerror());
            eli = followError(libname, elemlib, eli, searchPaths);
        } else {
	    // fprintf(stderr, "Unable to open: \'%s\', not found in search paths: \'%s\'\n",
		// elemlib.c_str(), searchPaths.c_str());
		}
    } else {
        // look for an info block
        std::string infoname = elemlib + "_eli";
        eli = (ElementLibraryInfo*) lt_dlsym(lt_handle, infoname.c_str());

        // Will not longer show this error here, will show it is Factory::loadLibrary()
        // if (NULL == eli) {
        //     if (showErrors) {
        //         char *old_error = strdup(lt_dlerror());
        //         fprintf(stderr, "Could not find ELI block %s in %s: %s\n",
        //                 infoname.c_str(), libname.c_str(), old_error);
        //         free(old_error);
        //     }
        // }
    }
    return eli;
}


const ElementLibraryInfo*
ElemLoader::loadCoreInfo()
{
    static const ElementInfoEvent events[] = {
        {NULL, NULL, NULL, 0}
    };

    static const ElementInfoModule modules[] = {
        {NULL, NULL, NULL, NULL, NULL, NULL, NULL}
    };

    static const ElementInfoPartitioner partitioners[] = {
        {"linear", "Partitions components by dividing Component ID space into roughly equal portions.  Components with sequential IDs will be placed close together.", NULL, NULL},
        {"roundrobin", "Partitions components using a simple round robin scheme based on ComponentID.  Sequential IDs will be placed on different ranks.", NULL, NULL},
        {"simple", "Simple partitioning scheme which attempts to partition on high latency links while balancing number of components per rank.", NULL, NULL},
        {"self", "Used when partitioning is already specified in the configuration.", NULL, NULL},
#ifdef HAVE_ZOLTAN
        {"zoltan", "Zoltan parallel partitioner", NULL, NULL},
#endif

        {NULL, NULL, NULL, NULL}
    };


    static const ElementLibraryInfo eli = {
        .name = "sst",
        .description = "SST Core elements",
        .components = NULL,
        .events = events,
        .introspectors = NULL,
        .modules = modules,
        .subcomponents = NULL,
        .partitioners = partitioners,
        .pythonModuleGenerator = NULL,
        .generators = NULL
    };
    return &eli;
}


extern "C" {
static int elemCB(const char *fname, void *vd)
{
    std::vector<std::string>* arr = (std::vector<std::string>*)vd;
    const char *base = strrchr(fname, '/') + 1; /* +1 to go past the / */
    if ( !strncmp("lib", base, 3) ) { /* Filter out directories and the like */
        arr->push_back(base + 3); /* skip past "lib" */
    }
    return 0;
}
}

std::vector<std::string>
ElemLoader::getPotentialElements()
{
    std::vector<std::string> res;
    lt_dlforeachfile(searchPaths.c_str(), elemCB, &res);
    return res;
}

}
