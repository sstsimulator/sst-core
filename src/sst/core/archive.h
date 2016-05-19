// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_ARCHIVE_H
#define SST_CORE_ARCHIVE_H

#include <string>

// Do NOT include a serialization header in this file.  The
// implementation of Archive requires some special Boost.Serialization
// magic not needed for the rest of the core, and it is handled in
// archive.cc.  There's no serialization interfaces in this header, so
// no include is required.

namespace SST {
    class Simulation;

    /**
     * \class Archive
     * Archives are used for checkpoint/restart.
     * NOT a Public API.
     */
    class Archive {
    public:
        /** Create a new Archive.
         * @param ttype - Type of archive (xml, text, bin)
         * @param filename - File to archive to or from
         */
        Archive(std::string, std::string);
        ~Archive();

        /** Save the simulation state to a file */
        void saveSimulation(Simulation* sim);
        /** Restore Simulation state from a file */
        Simulation* loadSimulation(void);
    private:
        Archive(); // do not implement
        std::string type;
        std::string filename;
    };

} //namespace SST

#endif // SST_CORE_ARCHIVE_H
