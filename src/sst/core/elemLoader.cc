// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"

#include "elemLoader.h"

#include "sst/core/eli/elementinfo.h"
#include "sst/core/component.h"
#include "sst/core/subcomponent.h"
#include "sst/core/part/sstpart.h"
#include "sst/core/sstpart.h"

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


ElemLoader::ElemLoader(const std::string& searchPaths) :
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


static std::vector<std::string> splitPath(const std::string& searchPaths)
{
    std::vector<std::string> paths;
    char * pathCopy = new char [searchPaths.length() + 1];
    std::strcpy(pathCopy, searchPaths.c_str());
    char *brkb = nullptr;
    char *p = nullptr;
    for ( p = strtok_r(pathCopy, ":", &brkb); p ; p = strtok_r(nullptr, ":", &brkb) ) {
        paths.push_back(p);
    }

    delete [] pathCopy;
    return paths;
}


static void followError(const std::string& libname, const std::string& elemlib, const std::string& searchPaths,
                        std::ostream& err_os)
{
    std::string so_path = libname + ".so";
    std::string fullpath;
    void *handle;

    std::vector<std::string> paths = splitPath(searchPaths);

    for ( std::string path : paths ) {
        struct stat sbuf;
        int ret;

        fullpath = path + "/" + so_path;
        ret = stat(fullpath.c_str(), &sbuf);
        if (ret == 0) break;
    }

    // This is a little weird, but always try the last path - if we
    // didn't succeed in the stat, we'll get a file not found error
    // from dlopen, which is a useful error message for the user.
    handle = dlopen(fullpath.c_str(), RTLD_NOW|RTLD_GLOBAL);
    if (nullptr == handle) {
        std::vector<char> err_str(1e6); //make darn sure we fit the str
        sprintf(err_str.data(),
          "Opening and resolving references for element library %s failed:\n" "\t%s\n",
        elemlib.c_str(), dlerror());
        err_os << (const char*) err_str.data();
    }
}

void
ElemLoader::loadLibrary(const std::string& elemlib, std::ostream& err_os)
{
    std::string libname = "lib" + elemlib;
    lt_dlhandle lt_handle;
    lt_handle = lt_dlopenadvise(libname.c_str(), loaderData->advise_handle);
    if (nullptr == lt_handle) {
      // The preopen module runs last and if the
        // component was found earlier, but has a missing symbol or
        // the like, we just get an amorphous "file not found" error,
        // which is totally useless...
        //Make darn sure we have enough space to hold the error message
        std::vector<char> err_str(1e6);
        sprintf(err_str.data(), "Opening element library %s failed: %s\n",
                elemlib.c_str(), lt_dlerror());
        err_os << (const char*) err_str.data();
        followError(libname, elemlib, searchPaths, err_os);
    }

    //loading a library can "wipe" previously loaded libraries depending
    //on how weak symbol resolution works in dlopen
    //rerun the loaders to make sure everything is still registered
    for (auto& libpair : ELI::LoadedLibraries::getLoaders()){
      //loop all the elements in the element lib
      for (auto& elempair : libpair.second){
        //loop all the loaders in the element
        for (auto* loader : elempair.second){
          loader->load();
        }
      }
    }
    return;
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
