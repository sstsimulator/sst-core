// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "elemLoader.h"

#include "sst/core/component.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/part/sstpart.h"
#include "sst/core/sstpart.h"
#include "sst/core/subcomponent.h"

#include <cstdio>
#include <cstring>
#include <climits>
#include <dirent.h>
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

/* This needs to happen before lt_dlinit() and sets up the preload
   libraries properly.  The macro declares an extern symbol, so if we
   do this in the sst namespace, the symbol is namespaced and then not
   found in linking.  So have this short function here.

   Only do this when building with element libraries.
   */
static void
preload_symbols(void)
{
    // README: This is only set if we are not building any elements
    // in the split up build this is now default so must always be called.
    //#ifndef __SST_BUILD_CORE_ONLY__
    //    LTDL_SET_PRELOADED_SYMBOLS();
    //#endif
}

namespace SST {

static std::vector<std::string>
splitPath(const std::string& searchPaths)
{
    std::vector<std::string> paths;
    char*                    pathCopy = new char[searchPaths.length() + 1];
    std::strcpy(pathCopy, searchPaths.c_str());
    char* brkb = nullptr;
    char* p    = nullptr;
    for ( p = strtok_r(pathCopy, ":", &brkb); p; p = strtok_r(nullptr, ":", &brkb) ) {
        paths.push_back(p);
    }

    delete[] pathCopy;
    return paths;
}

ElemLoader::ElemLoader(const std::string& searchPaths) : searchPaths(searchPaths)
{
    preload_symbols();
	verbose = false;

	const char* verbose_env = getenv("SST_CORE_DL_VERBOSE");
	if(nullptr != verbose_env) {
		verbose = atoi(verbose_env) > 0;
	}
}

ElemLoader::~ElemLoader() {}

void
ElemLoader::loadLibrary(const std::string& elemlib, std::ostream& err_os)
{
    std::string              libname = "lib" + elemlib;
    std::vector<std::string> paths   = splitPath(searchPaths);

    char* full_path     = new char[PATH_MAX];
    bool  found_element = false;

    for ( std::string& next_path : paths ) {
		  if(verbose) {
				printf("SST-DL: Searching: %s\n", next_path.c_str());
		  }

        DIR* current_dir = opendir(next_path.c_str());

        if ( current_dir ) {
            struct dirent* dir_file;

            while ( (dir_file = readdir(current_dir)) != nullptr ) {
					  if(verbose) {
							printf("SST-DL: Checking file types: %s\n", dir_file->d_name);
		  			  }

                if ( (dir_file->d_type | DT_REG) || (dir_file->d_type | DT_LNK) ) {
                    if ( !strncmp("lib", dir_file->d_name, 3) ) {
                        sprintf(full_path, "%s/%s", next_path.c_str(), dir_file->d_name);
								if(verbose) {
									printf("SST-DL: Attempting dynamic load of %s\n", full_path);
								}
                        void* handle = dlopen(full_path, RTLD_NOW | RTLD_GLOBAL);

                        if ( nullptr != handle ) {
                            found_element = true;

									 if(verbose) {
										printf("SST-DL: Load of %s was successful.\n", full_path);
									 }

                            // loading a library can "wipe" previously loaded libraries depending
                            // on how weak symbol resolution works in dlopen
                            // rerun the loaders to make sure everything is still registered
                            for ( auto& libpair : ELI::LoadedLibraries::getLoaders() ) {
                                // loop all the elements in the element lib
                                for ( auto& elempair : libpair.second ) {
                                    // loop all the loaders in the element
                                    for ( auto* loader : elempair.second ) {
                                        loader->load();
                                    }
                                }
                            }
                        } else {
									if(verbose) {
										printf("SST-DL: Load of %s failed, %s\n", full_path, dlerror());
									}
								}
                    }
                }
            }
        }
    }

    delete[] full_path;

    if ( !found_element ) { err_os << "Error: unable to find \"" << elemlib << "\" element library\n"; }

    return;
}

void
ElemLoader::getPotentialElements(std::vector<std::string>& potential_elements)
{
    std::vector<std::string> paths = splitPath(searchPaths);

    for ( std::string& next_path : paths ) {
        DIR* current_dir = opendir(next_path.c_str());

        if ( current_dir ) {
            struct dirent* dir_file;
            while ( (dir_file = readdir(current_dir)) != nullptr ) {
                // ensure we are only processing normal files and nothing weird
                if ( (dir_file->d_type | DT_REG) || (dir_file->d_type | DT_LNK) ) {
                    char* current_file = new char[strlen(dir_file->d_name) + 1];
                    std::strcpy(current_file, dir_file->d_name);

                    // does the path start with "lib" required for SST
                    if ( !strncmp("lib", current_file, 3) ) {
                        // find out if we have an extension
                        char* find_ext = strrchr(current_file, '.');

                        if ( nullptr != find_ext ) {
                            // need to strip the extension from the file name
                            find_ext[0] = '\0';
                            potential_elements.push_back(current_file + 3);
                        }
                    }

                    delete[] current_file;
                }
            }

            closedir(current_dir);
        }
    }
}

} // namespace SST
