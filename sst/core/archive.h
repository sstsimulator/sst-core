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

namespace SST {
    class Simulation;

    class Archive {
    public:
        Archive(std::string, std::string);
        ~Archive();

        void SaveSimulation(Simulation* sim);
        Simulation* LoadSimulation(void);
    private:
        Archive(); // do not implement
        std::string type;
        std::string filename;
    };

} //namespace SST

#endif // SST_ARCHIVE_H
