// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "elemLoader.h"

#include "sst/core/component.h"
#include "sst/core/eli/elementinfo.h"
#include "sst/core/namecheck.h"
#include "sst/core/part/sstpart.h"
#include "sst/core/sstpart.h"
#include "sst/core/subcomponent.h"

#include <climits>
#include <cstdio>
#include <cstring>
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

namespace {

template <class BaseType>
void
checkForValidParamNames(const std::string& libname)
{
    auto* lib = SST::ELI::InfoDatabase::getLibrary<BaseType>(libname);
    if ( lib ) {
        const auto& info_map = lib->getMap();
        for ( auto& x : info_map ) {
            const std::string comp_name(x.first);
            const auto&       param_map = x.second->getValidParams();
            for ( auto& param : param_map ) {
                if ( !SST::NameCheck::isParamNameValid(param.name) ) {
                    printf(
                        "WARNING: Element %s.%s has parameter with an invalid name: %s\n", libname.c_str(),
                        comp_name.c_str(), param.name);
                }
            }
        }
    }
}

template <class BaseType>
void
checkForValidPortNames(const std::string& libname)
{
    auto* lib = SST::ELI::InfoDatabase::getLibrary<BaseType>(libname);
    if ( lib ) {
        const auto& info_map = lib->getMap();
        for ( auto& x : info_map ) {
            const std::string comp_name(x.first);
            const auto&       port_map = x.second->getValidPorts();
            for ( auto& port : port_map ) {
                if ( !SST::NameCheck::isPortNameValid(port.name) ) {
                    printf(
                        "WARNING: Element %s.%s has port with an invalid name: %s\n", libname.c_str(),
                        comp_name.c_str(), port.name);
                }
            }
        }
    }
}

template <class BaseType>
void
checkForValidSlotNames(const std::string& libname)
{
    auto* lib = SST::ELI::InfoDatabase::getLibrary<BaseType>(libname);
    if ( lib ) {
        const auto& info_map = lib->getMap();
        for ( auto& x : info_map ) {
            const std::string comp_name(x.first);
            const auto&       slot_map = x.second->getSubComponentSlots();
            for ( auto& slot : slot_map ) {
                if ( !SST::NameCheck::isSlotNameValid(slot.name) ) {
                    printf(
                        "WARNING: Element %s.%s has slot with an invalid name: %s\n", libname.c_str(),
                        comp_name.c_str(), slot.name);
                }
            }
        }
    }
}


} // namespace

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

ElemLoader::ElemLoader(const std::string& searchPaths) :
    searchPaths(searchPaths),
    verbose(false),
    bindPolicy(RTLD_LAZY | RTLD_GLOBAL)
{

    const char* verbose_env = getenv("SST_CORE_DL_VERBOSE");
    if ( nullptr != verbose_env ) { verbose = atoi(verbose_env) > 0; }

    const char* bind_env = getenv("SST_CORE_DL_BIND_POLICY");
    if ( (nullptr != bind_env) && ((!strcmp(bind_env, "now")) || (!strcmp(bind_env, "NOW"))) ) {
        bindPolicy = RTLD_NOW | RTLD_GLOBAL;
    }
}

ElemLoader::~ElemLoader() {}


void
ElemLoader::loadLibrary(const std::string& elemlib, std::ostream& err_os)
{
    std::vector<std::string> paths = splitPath(searchPaths);

    char* full_path     = new char[PATH_MAX];
    bool  found_element = false;

    // We will keep track of error messages.  In general, we will only
    // print them if the library completely fails to load. When
    // verbose is turned on, we will simply print all the
    // errors/warnings/info whether things succeed or not.
    std::vector<std::string> error_msgs;

    for ( std::string const& next_path : paths ) {
        if ( verbose ) { printf("SST-DL: Searching: %s\n", next_path.c_str()); }

        if ( next_path.back() == '/' ) {
            snprintf(full_path, PATH_MAX, "%slib%s.so", next_path.c_str(), elemlib.c_str());
        }
        else {
            snprintf(full_path, PATH_MAX, "%s/lib%s.so", next_path.c_str(), elemlib.c_str());
        }

        if ( verbose ) { printf("SST-DL: Attempting to load %s\n", full_path); }

        // use a global bind policy read from environment, default to RTLD_LAZY
        void* handle = dlopen(full_path, bindPolicy);

#ifdef SST_COMPILE_MACOSX
        if ( nullptr == handle ) {
            if ( verbose ) { printf("SST-DL: Loading failed for %s, error: %s\n", full_path, dlerror()); }
            else {
                // Check to see if file exists.  If not, we don't need
                // to record the error message.
                struct stat sb;
                if ( 0 == stat(full_path, &sb) ) {
                    // File found, record error
                    error_msgs.emplace_back("SST-DL: Loading failed for ");
                    error_msgs.back() =
                        error_msgs.back() + std::string(full_path) + ", error:\n" + std::string(dlerror());
                }
            }
        }

        // macOS will also allow files to use the dylib extension so this
        // must also be checked. But only check this if the .so attempt
        // failed first (because we may have had a successful load already)
        // this implies ordering of .so before .dylib in priority.

        if ( nullptr == handle ) {
            if ( next_path.back() == '/' ) {
                snprintf(full_path, PATH_MAX, "%slib%s.dylib", next_path.c_str(), elemlib.c_str());
            }
            else {
                snprintf(full_path, PATH_MAX, "%s/lib%s.dylib", next_path.c_str(), elemlib.c_str());
            }

            if ( verbose ) { printf("SST-DL: Attempting to load %s\n", full_path); }

            // use a global bind policy read from environment, default to RTLD_LAZY
            handle = dlopen(full_path, bindPolicy);
        }
#endif

        if ( nullptr == handle ) {
            if ( verbose ) { printf("SST-DL: Loading failed, error: %s\n", dlerror()); }
            else {
                // Check to see if file exists.  If not, we don't need
                // to record the error message.
                struct stat sb;
                if ( 0 == stat(full_path, &sb) ) {
                    // File found, record error
                    error_msgs.emplace_back("SST-DL: Loading failed for ");
                    error_msgs.back() =
                        error_msgs.back() + std::string(full_path) + ", error: " + std::string(dlerror());
                }
            }
        }
        else {
            if ( verbose ) { printf("SST-DL: Load was successful.\n"); }

            found_element = true;

            for ( auto& libpair : ELI::LoadedLibraries::getLoaders() ) {
                // loop all the elements in the element lib
                for ( auto& elempair : libpair.second ) {
                    // loop all the loaders in the element
                    for ( auto* loader : elempair.second ) {
                        loader->load();
                    }
                }
            }

            // exit the search loop, we have found the library we tried to load
            break;
        }
    }

    delete[] full_path;

    if ( !found_element ) {
        err_os << "Error: unable to find \"" << elemlib << "\" element library\n";
        if ( !verbose ) {
            for ( auto& x : error_msgs ) {
                err_os << x << std::endl;
            }
        }
    }
    else {
        // If we found an element, but have error messages from
        // loading a file that existed, print that as a warning.
        if ( !verbose && !error_msgs.empty() ) {
            err_os << "Warning: library " << elemlib
                   << " was successfully loaded, but there was a failed attempt at loading a different version of the "
                      "library before the successful load.  Error message follows:\n";
            for ( auto& x : error_msgs ) {
                err_os << x << std::endl;
            }
        }
    }

    // Need to check the element library to make sure names of params,
    // etc. are valid
    checkForValidParamNames<Component>(elemlib);
    checkForValidPortNames<Component>(elemlib);
    checkForValidSlotNames<Component>(elemlib);
    checkForValidParamNames<SubComponent>(elemlib);
    checkForValidPortNames<SubComponent>(elemlib);
    checkForValidSlotNames<SubComponent>(elemlib);

    return;
}

void
ElemLoader::getPotentialElements(std::vector<std::string>& potential_elements)
{
    std::vector<std::string> paths = splitPath(searchPaths);

    for ( std::string const& next_path : paths ) {
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
    // Always add in the sst library, since it's built into the sst
    // and sst-info executables
    potential_elements.push_back("sst");
}

} // namespace SST
