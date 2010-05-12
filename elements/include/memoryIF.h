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


#ifndef _MEMORYIF_H
#define _MEMORYIF_H

template < typename addrT = unsigned long,
            typename cookieT = void*,
            typename dataT = unsigned long
         >
class MemoryIF
{
    public: // types
        typedef addrT    addr_t;
        typedef cookieT  cookie_t;
        typedef dataT    data_t;
        virtual ~MemoryIF() {;}

    public: // functions
        virtual bool read( addr_t, cookie_t ) = 0;
        virtual bool write( addr_t, cookie_t ) = 0;
        virtual bool read( addr_t, data_t*, cookie_t ) = 0;
        virtual bool write( addr_t, data_t*, cookie_t ) = 0;
        virtual bool popCookie( cookie_t& ) = 0;
};

#endif
