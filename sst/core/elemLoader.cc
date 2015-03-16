// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include "elemLoader.h"

#include <ltdl.h>
#include <vector>

#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

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

#include <stdio.h>

/* This needs to happen before lt_dlinit() and sets up the preload
   libraries properly.  The macro declares an extern symbol, so if we
   do this in the sst namespace, the symbol is namespaced and then not
   found in linking.  So have this short function here. */
static void preload_symbols(void) {
    LTDL_SET_PRELOADED_SYMBOLS();
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


static ElementLibraryInfo* followError(std::string libname, std::string elemlib, ElementLibraryInfo* eli, std::string searchPaths)
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
        }
    } else {
        // look for an info block
        std::string infoname = elemlib + "_eli";
        eli = (ElementLibraryInfo*) lt_dlsym(lt_handle, infoname.c_str());

        if (NULL == eli) {
            char *old_error = strdup(lt_dlerror());
            fprintf(stderr, "Could not find ELI block %s in %s: %s\n",
                    infoname.c_str(), libname.c_str(), old_error);
            free(old_error);
        }
    }
    return eli;
}
}
