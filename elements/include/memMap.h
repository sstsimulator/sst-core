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


#ifndef _MEMMAP_H
#define _MEMMAP_H

#include <map>

template <typename keyT, typename rangeT, typename valT >
class MemMap
{
    private: // types
        typedef std::multimap< keyT, std::pair< rangeT, valT > > map_t;

    public:
        class iterator;
        friend class iterator;
        class iterator {
            public: // functions
                iterator( ) : m_iter( NULL ) {
                }
                iterator( MemMap* mm ) : m_iter( mm->m_map.begin() ) {
                }
                iterator( MemMap* mm, bool ) : m_iter( mm->m_map.end() ) {
                }
                inline valT operator++(int) { 
                    valT tmp = value();
                    ++m_iter; 
                    return tmp;
                }
                inline valT operator*() const {
                    return value();
                }
                inline bool operator!=(const iterator& rv ) const {
                    return m_iter != rv.m_iter; 
                } 

            private: // function
                inline valT value() const { 
                    return m_iter->second.second;
                }

            private: // data
                typename MemMap::map_t::iterator m_iter;
        };
        inline iterator begin() { return iterator( this ); }
        inline iterator end() { return iterator( this, true); }

    public: // functions
        bool insert( keyT key, rangeT range, valT val );
        valT* find( keyT key );
        bool erase( keyT key );
        bool empty( );
        size_t size( );

    private: // data
        map_t   m_map;
};

template < typename keyT, typename rangeT, typename valT >
inline bool MemMap< keyT, rangeT, valT >::empty( )
{
    return m_map.empty();
}

template < typename keyT, typename rangeT, typename valT >
inline size_t MemMap< keyT, rangeT, valT >::size( )
{
    return m_map.size();
}

template < typename keyT, typename rangeT, typename valT >
inline bool
MemMap< keyT, rangeT, valT >::insert( keyT key, rangeT range, valT val )
{
    //printf("insert key=%#lx range=%#lx\n",key,range);
    if ( range != 0 && ( find( key ) || find( key + range ) ) ) {
        return true;
    } 
    //printf("insert OK key=%#lx range=%#lx\n",key,range);
    m_map.insert( std::make_pair( key, std::make_pair( range, val) ) );
    return false;
}

template < typename keyT, typename rangeT, typename valT >
inline valT* MemMap< keyT, rangeT, valT >::find( keyT key )
{
    //printf("find key=%#lx\n",key);

    if ( m_map.empty() ) {
        return NULL;
    }

    // upper_bound will be the base key of the "next" region, hench "--" 
    typename map_t::iterator iter = m_map.upper_bound( key ); 

    --iter;
    std::pair< typename map_t::iterator, typename map_t::iterator> range;
    range = m_map.equal_range( iter->first );

    for ( iter = range.first; iter != range.second; ++iter )
    {
        //printf("find base=%lu range=%lu\n",iter->first, iter->second.first);
        if ( iter->second.first ) {
            if ( key >= iter->first && 
                        key < ( iter->first + iter->second.first ) )
            {
                return &iter->second.second;
            }
        }
    } 
    return NULL;
}

#if 0
template <typename keyT, typename rangeT, typename valT >
inline bool MemMap< keyT, rangeT, valT >::erase( keyT key )
{
    if ( m_map.find( key ) == m_map.end() ) {
        return true;
    }
    m_map.erase( key );
    return false;
}
#endif

#endif
