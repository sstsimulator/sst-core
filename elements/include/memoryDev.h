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



#ifndef _MEMORY_DEV_H
#define _MEMORY_DEV_H

#include <memoryIF.h>
#include <memoryChannel.h>

#define MEMORYDEV_DBG 1 
#ifndef MEMORYDEV_DBG
#define MEMORYDEV_DBG 0
#endif

using namespace SST;

template < typename addrT = unsigned long,
            typename cookieT = unsigned long, 
            typename dataT = unsigned long,
            typename fooT = std::pair< cookieT, dataT* >  >
class MemoryDev : 
    protected MemoryChannel< addrT, fooT*, dataT >,
    public MemoryIF< addrT, cookieT >
{
    public: // types

        typedef addrT addr_t;
        typedef dataT data_t;
        typedef cookieT cookie_t;
        typedef MemoryChannel< addrT, fooT*, dataT> dev_t;
        typedef typename dev_t::event_t event_t;
        typedef fooT foo_t;
    
    public: // functions 

        MemoryDev( Component& comp, Component::Params_t params,
                                                        std::string name );
        virtual bool read( addr_t, cookie_t );
        virtual bool write( addr_t, cookie_t );
        virtual bool read( addr_t, data_t*, cookie_t );
        virtual bool write( addr_t, data_t*, cookie_t );
        virtual bool popCookie( cookie_t& );
        
    private: // functions 
        bool send( addr_t, data_t*, cookie_t, typename event_t::reqType_t );

    private: // data
        Log< MEMORYDEV_DBG >&   m_dbg;
};

#define MEMORYDEV( retType ) \
template < typename addrT, typename cookieT, typename dataT, typename fooT >\
retType MemoryDev< addrT, cookieT, dataT, fooT >\

#include <memoryDev2.h>

#endif
