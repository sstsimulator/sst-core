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


#include <sst_config.h>
#include <sst/core/serialization.h>

#include <sst/core/lookupTableManager.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sst/core/lookupTable.h>




namespace SST {

void* LookupTableManager::registerTable(std::string key, LookupTableBuilder *builder)
{
    /* TODO: Lock map during lookup */
    LookupTableInfo& table = tables[key];

    if ( !table.initialized && builder ) {
        errno = 0;
        table.build(builder);
    }

    if ( builder) delete builder;

    return table.ptr;
}


int LookupTableManager::LookupTableInfo::build(LookupTableBuilder *builder)
{
    /* TODO: Lock table */
    if ( initialized ) return 0;
    initialized = true;

    errno = 0;
    size = builder->getSize();
    if ( 0 == size ) {
        shutdown();
        return 1;
    } else {
        size_t pagesize = (size_t)sysconf(_SC_PAGE_SIZE);
        size_t npages = size / pagesize;
        if ( size % pagesize ) npages++;
        realsize = npages * pagesize;

        int ret = posix_memalign(&ptr, pagesize, realsize);
        if ( ret ) {
            errno = ret;
            shutdown();
            return 1;
        } else {
            ret = builder->populateTable(ptr, size);
            if ( ret ) {
                shutdown();
                return 1;
            } else {
                mprotect(ptr, realsize, PROT_READ);
            }
        }
    }
    return 0;
}


void LookupTableManager::LookupTableInfo::shutdown()
{
    if ( ptr ) free(ptr);
    ptr = NULL;
    size = realsize = 0;
}

}
