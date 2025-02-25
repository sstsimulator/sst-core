// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/eli/elibase.h"

namespace SST::ELI {

/**************************************************************************
  BaseElementInfo class functions
**************************************************************************/

std::unique_ptr<LoadedLibraries::LibraryMap> LoadedLibraries::loaders_ {};

bool
LoadedLibraries::addLoader(
    const std::string& lib, const std::string& name, const std::string& alias, LibraryLoader* loader)
{
    if ( !loaders_ ) { loaders_ = std::unique_ptr<LibraryMap>(new LibraryMap); }
    (*loaders_)[lib][name].push_back(loader);
    if ( !alias.empty() ) (*loaders_)[lib][alias].push_back(loader);
    return true;
}

const LoadedLibraries::LibraryMap&
LoadedLibraries::getLoaders()
{
    if ( !loaders_ ) { loaders_ = std::unique_ptr<LibraryMap>(new LibraryMap); }
    return *loaders_;
}

bool
LoadedLibraries::isLoaded(const std::string& name)
{
    if ( loaders_ ) { return loaders_->find(name) != loaders_->end(); }
    else {
        return false; // nothing loaded yet
    }
}

} // namespace SST::ELI
