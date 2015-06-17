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

#ifndef SST_CORE_CORE_LOOKUPTABLE_H
#define SST_CORE_CORE_LOOKUPTABLE_H

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>


namespace SST {
class LookupTableBuilder {
public:
    LookupTableBuilder() { }
    ~LookupTableBuilder() { }

    /**
     * @return the size of the memory buffer desired
     */
    virtual size_t getSize() = 0;

    /**
     * populateTable - Responsible for filling in the data for the lookup
     *                 table.
     * @param ptr - Pointer to memory buffer to fill.
     * @param size - Size of the memory buffer.
     * @return 0 on success, 1 on error
     * Allowed to set 'errno'
     */
    virtual int populateTable(void *ptr, size_t size) = 0;
};



/**
 * LookupTable Builder which just reads in a file from disk.
 */
class SimpleLookupTableBuilder : public LookupTableBuilder {
protected:
    std::string fname;
public:
    /**
     * @param filename - name of file to open
     * @param size - optional argument.  Receives the size of the file.
     */
    SimpleLookupTableBuilder(const std::string &filename, size_t *size = NULL) : LookupTableBuilder() {
        fname = filename;
        if ( size ) *size = getSize();
    }

    virtual size_t getSize();
    virtual int populateTable(void *ptr, size_t size);

};


}


#endif
