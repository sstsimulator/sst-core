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


#ifndef SST_ARCHIVE_H
#define SST_ARCHIVE_H

#include <string>

// Do NOT include a serialization header in this file.  The
// implementation of Archive requires some special Boost.Serialization
// magic not needed for the rest of the core, and it is handled in
// archive.cc.  There's no serialization interfaces in this header, so
// no include is required.

namespace SST {
    class Simulation;

    class Archive {
    public:
        Archive(std::string, std::string);
        ~Archive();

        void saveSimulation(Simulation* sim);
        Simulation* loadSimulation(void);
    private:
        Archive(); // do not implement
        std::string type;
        std::string filename;
    };

} //namespace SST

#endif // SST_ARCHIVE_H
