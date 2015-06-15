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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sst/core/lookupTable.h>




namespace SST {

size_t SimpleLookupTableBuilder::getSize()
{
    int ret = 0;
    struct stat buf;

    ret = stat(fname.c_str(), &buf);
    if ( ret != 0 )
        return 0;

    return buf.st_size;
}


int SimpleLookupTableBuilder::populateTable(void *ptr, size_t size)
{
    FILE *fp = fopen(fname.c_str(), "r");
    if ( !fp )
        return 1;

    size_t nitems = fread(ptr, 1, size, fp);

    fclose(fp);

    if ( nitems < size ) return 1;

    return 0;
}






}
