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

#ifndef _SST_CORE_ELEMLOADER_H
#define _SST_CORE_ELEMLOADER_H

#include <string>
#include <vector>
#include <map>

namespace SST {

struct LoaderData;
struct ElementInfoGenerator;

/** Class to load Element Libraries */
class ElemLoader {
    LoaderData *loaderData;
    std::string searchPaths;
public:
    /** Create a new ElementLoader with a given searchpath of directories */
    ElemLoader(const std::string& searchPaths);
    ~ElemLoader();

    /** Attempt to load a library
     * @param elemlib - The name of the Element Library to load
     * @param err_os - Where to print errors associated with attempting to find and load the library
     * @return Informational structure of the library, or nullptr if it failed to load.
     */
    void loadLibrary(const std::string& elemlib, std::ostream& err_os);

    /**
     * Returns a list of potential element libraries in the search path
     */
    std::vector<std::string> getPotentialElements();
};

}

#endif /* _SST_CORE_ELEMLOADER_H */
