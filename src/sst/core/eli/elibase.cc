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

bool
LoadedLibraries::addLoader(
    const std::string& lib, const std::string& name, const std::string& alias, LibraryLoader* loader)
{
    std::shared_ptr<LibraryLoader> shared_loader(loader);

    auto& library = getLoaders()[lib];
    if ( !alias.empty() && alias != name ) library[alias].push_back(shared_loader);
    library[name].push_back(std::move(shared_loader));
    return true;
}

} // namespace SST::ELI
