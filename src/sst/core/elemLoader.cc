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
#include "sst/core/oldELI.h"
#include "sst/core/elementinfo.h"
#include "sst/core/component.h"
#include "sst/core/subcomponent.h"
#include <sst/core/part/sstpart.h>
#include <sst/core/sstpart.h>
#include <sst/core/model/element_python.h>

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

void
ElemLoader::loadOldELI(const ElementLibraryInfo* eli,
                       std::map<std::string, const ElementInfoGenerator*>& found_generators)
{
  if (eli == nullptr) return;

  OldELITag tag{eli->name};

  std::string elemlib = eli->name;

  if (NULL != eli->components) {
    const auto *eic = eli->components;
    auto* infolib = ELI::InfoDatabase::getLibrary<Component>(elemlib);
    auto* factlib = Component::getBuilderLibrary(elemlib);
    if (!infolib || !factlib){
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,
                               "failed loading old ELI '%s' component info", elemlib.c_str());
    }

    while (NULL != eic->name) {
      factlib->addBuilder(eic->name, new Component::DerivedBuilder<OldELITag>(eic->alloc));
      infolib->addInfo(eic->name,new Component::BuilderInfo(elemlib,eic->name,tag,eic));
    }
  }

  if (NULL != eli->modules) {
    const ElementInfoModule *eim = eli->modules;
    auto* infolib = ELI::InfoDatabase::getLibrary<Module>(elemlib);
    auto* withlib = ELI::BuilderDatabase::getLibrary<Module,Component*,SST::Params&>(elemlib);
    auto* wolib = ELI::BuilderDatabase::getLibrary<Module,SST::Params&>(elemlib);
    if (!infolib || !withlib || !wolib){
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,
                               "failed loading old ELI '%s' module info", elemlib.c_str());
    }

    while (NULL != eim->name) {
      auto* info = new Module::BuilderInfo(elemlib,eim->name,tag,eim);
      infolib->addInfo(eim->name,info);

      auto* withComp = new Module::DerivedBuilder<OldELITag,Component*,SST::Params&>(eim->alloc_with_comp);
      withlib->addBuilder(eim->name, withComp);

      auto* withoutComp = new Module::DerivedBuilder<OldELITag,SST::Params&>(eim->alloc);
      wolib->addBuilder(eim->name, withoutComp);
      eim++;
    }
  }

  if (NULL != eli->subcomponents ) {
    const auto *eis = eli->subcomponents;
    auto* infolib = ELI::InfoDatabase::getLibrary<SubComponent>(elemlib);
    auto* factlib = SubComponent::getBuilderLibrary(elemlib);
    if (!infolib || !factlib){
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,
                               "failed loading old ELI '%s' subcomponent info", elemlib.c_str());
    }

    while (NULL != eis->name) {
      factlib->addBuilder(eis->name, new SubComponent::DerivedBuilder<OldELITag>(eis->alloc));
      infolib->addInfo(eis->name,new SubComponent::BuilderInfo(elemlib,eis->name,tag,eis));
      eis++;
    }
  }

  if (NULL != eli->partitioners) {
    const auto *eis = eli->partitioners;
    auto* infolib = ELI::InfoDatabase::getLibrary<Partition::SSTPartitioner>(elemlib);
    auto* factlib = Partition::SSTPartitioner::getBuilderLibrary(elemlib);
    if (!infolib || !factlib){
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,
                               "failed loading old ELI '%s' partitioner info", elemlib.c_str());
    }
    while (NULL != eis->name) {
      factlib->addBuilder(eis->name, new Partition::SSTPartitioner::DerivedBuilder<OldELITag>(eis->func));
      infolib->addInfo(eis->name, new Partition::SSTPartitioner::BuilderInfo(elemlib,eis->name,tag,eis));
      eis++;
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

  if (NULL != eli->pythonModuleGenerator ) {
    auto* factlib = SSTElementPythonModule::getBuilderLibrary(elemlib);
    auto* infolib = ELI::InfoDatabase::getLibrary<SSTElementPythonModule>(elemlib);

    if (!infolib || !factlib){
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,
                               "failed loading old ELI '%s' Python module info", elemlib.c_str());
    }

    auto* fact = new SSTElementPythonModule::DerivedBuilder<SSTElementPythonModuleOldELI>(eli->pythonModuleGenerator);
    factlib->addBuilder(elemlib, fact);


    auto* info = new SSTElementPythonModule::BuilderInfo(elemlib,elemlib,tag,eli);
    infolib->addInfo(elemlib,info);
  }

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

    //loading a library can "wipe" previously loaded libraries depending
    //on how weak symbol resolution works in dlopen
    //rerun the loaders to make sure everything is still registered
    for (auto& libpair : ELI::LoadedLibraries::getLoaders()){
      for (auto& elempair : libpair.second){
        //call the loader function
        elempair.second();
      }
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
