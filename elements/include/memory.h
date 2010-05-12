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


#ifndef _MEMORY_H
#define _MEMORY_H

#ifndef MEMORY_DBG
#define MEMORY_DBG 0
#endif

#define _MEMORY_DBG( fmt, args... ) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)

#include <queue>
#include <memoryDev.h>
#include <memMap.h>

template < typename addrT = uint64_t, 
            typename cookieT = unsigned long,
            typename dataT = unsigned long  >
class Memory :
    public MemoryIF< addrT, cookieT >
{
    public: // types
        typedef MemoryDev< addrT, cookieT > dev_t;
        typedef addrT                       addr_t;
        typedef addr_t                      length_t;
        typedef cookieT                     cookie_t;
        typedef dataT                       data_t;

    public: // functions
        Memory();
        bool devAdd( dev_t*, addr_t, addr_t );

        virtual bool read( addr_t, cookie_t );
        virtual bool write( addr_t, cookie_t );
        virtual bool read( addr_t, data_t*, cookie_t );
        virtual bool write( addr_t, data_t*, cookie_t );
        virtual bool popCookie( cookie_t& );
        virtual bool map( addr_t, addr_t, length_t );

    private: // type
        typedef MemMap< addr_t, addr_t, dev_t* >  devMap_t;
        typedef MemMap< addr_t, addr_t, std::pair< addr_t, addr_t > >  memMap_t;

    private: // functions
        addr_t calcAddr( addr_t );

    private: // data
        std::deque< cookie_t >  m_cookieQ;
        devMap_t                m_devMap; 
        memMap_t                m_memMap; 
        Log< MEMORY_DBG >&      m_dbg;
};

template < typename addrT, typename cookieT, typename dataT >
Memory< addrT, cookieT, dataT >::Memory() :
    m_dbg( *new Log< MEMORY_DBG >( "Memory::" ) )
{
}

template < typename addrT, typename cookieT, typename dataT >
bool Memory< addrT, cookieT, dataT >::devAdd( dev_t* dev,
                                    addr_t addr, length_t length )
{
    _MEMORY_DBG( "addr=%#lx length=%#lx\n", addr, length );
    return m_devMap.insert( addr, length, dev );
}

template < typename addrT, typename cookieT, typename dataT >
inline bool 
Memory< addrT, cookieT, dataT >::map( addr_t from, 
                                    addr_t to, length_t length )
{
    _MEMORY_DBG( "from=%#lx to=%#lx length=%#lx\n", from, to, length );
    return m_memMap.insert( from, length, std::make_pair( from, to ) );
}

template < typename addrT, typename cookieT, typename dataT >
inline bool 
Memory< addrT, cookieT, dataT >::read( addr_t addr, cookie_t cookie = NULL )
{
    addr = calcAddr( addr );
    dev_t**  dev = m_devMap.find( addr );
    if ( dev == NULL ) {
        _abort( Memory, "m_devMap.find( %#lx ) failed\n", (unsigned long)addr);
    }
    return (*dev)->read( addr, cookie );
}

template < typename addrT, typename cookieT, typename dataT >
inline bool 
Memory< addrT, cookieT, dataT >::write( addr_t addr, cookie_t cookie = NULL )
{
    addr = calcAddr( addr );
    dev_t**  dev = m_devMap.find( addr );
    if ( dev == NULL ) {
        _abort( Memory, "m_devMap.find( %#lx ) failed\n", (unsigned long)addr);
    }
    return (*dev)->write( addr, cookie );
}

template < typename addrT, typename cookieT, typename dataT >
inline bool 
Memory< addrT, cookieT, dataT >::read( addr_t addr, data_t* data, 
                                                    cookie_t cookie = NULL )
{
    addr = calcAddr( addr );
    dev_t**  dev = m_devMap.find( addr );
    if ( dev == NULL ) {
        _abort( Memory, "m_devMap.find( %#lx ) failed\n", (unsigned long)addr);
    }
    return (*dev)->read( addr, data, cookie );
}

template < typename addrT, typename cookieT, typename dataT >
inline bool 
Memory< addrT, cookieT, dataT >::write( addr_t addr, data_t* data,
                                                    cookie_t cookie = NULL )
{
    addr = calcAddr( addr );
    dev_t**  dev = m_devMap.find( addr );
    if ( dev == NULL ) {
        _abort( Memory, "m_devMap.find( %#lx ) failed\n", (unsigned long)addr);
    }
    return (*dev)->write( addr, data, cookie );
}


template < typename addrT, typename cookieT, typename dataT >
inline bool 
Memory< addrT, cookieT, dataT >::popCookie( cookieT& cookie )
{
    if ( ! m_cookieQ.empty() ) {
        cookie = m_cookieQ.front();
        m_cookieQ.pop_front();
        return true;
    }

    bool ret = false;
    typename devMap_t::iterator iter = m_devMap.begin();
    for ( ; iter != m_devMap.end(); iter++ )
    {
        cookieT _cookie;
        if ( (*iter)->popCookie( _cookie ) ) {
            if ( ! ret ) {
                cookie = _cookie;
                ret = true;
            } else {
                m_cookieQ.push_back( _cookie );
            }
        }
    }
    return ret;
}

template < typename addrT, typename cookieT, typename dataT >
inline addrT 
Memory< addrT, cookieT, dataT >::calcAddr( addrT addr )
{
    if ( m_memMap.empty() ) {
        _MEMORY_DBG( "addr=%#lx\n", addr );
        return addr;
    }
    
    std::pair< addr_t, addr_t >* tmp;
    tmp = m_memMap.find( addr );
    if ( tmp == NULL ) {
        _abort( Memory, "m_memMap.find( %#lx ) failed\n", (unsigned long)addr);
    }

    addr_t new_addr = ( addr - (*tmp).first ) + (*tmp).second; 
    _MEMORY_DBG("%#lx -> %#lx\n", addr, new_addr );

    return new_addr;
}

#endif
