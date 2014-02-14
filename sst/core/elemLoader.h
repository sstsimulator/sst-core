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

#ifndef _SST_CORE_ELEMLOADER_H
#define _SST_CORE_ELEMLOADER_H

#include <string>
#include <sst/core/element.h>

namespace SST {

struct LoaderData;

class ElemLoader {
    LoaderData *loaderData;
    std::string searchPaths;
public:
    ElemLoader(const std::string &searchPaths);
    ~ElemLoader();

    const ElementLibraryInfo* loadLibrary(const std::string &elemlib, bool showErrors);
};

}

#endif /* _SST_CORE_ELEMLOADER_H */
