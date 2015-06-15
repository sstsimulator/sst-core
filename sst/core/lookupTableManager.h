// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_CORE_LOOKUPTABLEMANAGER_H
#define SST_CORE_CORE_LOOKUPTABLEMANAGER_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

#include <sys/mman.h>
#include <map>

#include <sst/core/lookupTable.h>

namespace SST {
class LookupTableManager {
private:

    struct LookupTableInfo {
        /* TODO: mutex */
        bool initialized;
        size_t realsize;
        size_t size;
        void *ptr;

        LookupTableInfo() : initialized(false), realsize(0), size(0), ptr(NULL)
        { }

        int build(LookupTableBuilder *builder);
        void shutdown();
    };


    std::map<std::string, LookupTableInfo> tables;
    /* TODO: mutex */

public:
    LookupTableManager() { }

    ~LookupTableManager() {
        for ( auto &&entry : tables ) {
            LookupTableInfo &table = entry.second;
            table.shutdown();
        }
    }

    /**
     * Register and find a Lookup Table based off of a key
     * @param key String to use to uniqely identify the lookup table to find
     * @param builder Pointer to a subclass instance of LookupTableBuilder.
     *        Manager will take ownership of this pointer.
     * @return NULL on error, or pointer to lookup table memory area
     */
    void* registerTable(std::string key, LookupTableBuilder *builder);


};

}


#endif
