// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



#ifndef _SST_POOL_H
#define _SST_POOL_H

namespace SST {

template <typename ObjectT>
class Pool {
    public:
        ObjectT *Alloc() {
            return new ObjectT;
        } 
        void Dealloc( ObjectT *obj ) {
            delete obj;
        } 
    private:
};

}

#endif
