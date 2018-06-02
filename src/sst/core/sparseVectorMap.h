// -*- c++ -*-

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SPARSEVECTORMAP_H
#define SST_CORE_SPARSEVECTORMAP_H

#include "sst/core/sst_types.h"
#include <sst/core/serialization/serializable.h>

#include <vector>

namespace SST {

    
template <typename keyT, typename classT = keyT>
class SparseVectorMap {
private:
    friend class SST::Core::Serialization::serialize<SparseVectorMap<keyT,classT> >;
    
    std::vector<classT> data;
    int binary_search_insert(keyT id) const
    {
        // For insert, we've found the right place when id < n && id >
        // n-1.  We then insert at n.

        int size = data.size();

        // Check to see if it goes top or bottom
        if ( size == 0 ) return 0;
        if ( id < data[0].key() ) return 0;
        if ( id > data[size-1].key() ) return size;
        
        int bottom = 0;
        int top = size - 1;
        int middle;

        while ( bottom <= top ) {
            middle = bottom + (top - bottom) / 2;
            if ( id == data[middle].key() ) return -1;  // Already in map
            if ( id < data[middle].key() ) {
                // May be the right place, need to see if id is greater
                // than the one before the current middle.  If so, then we
                // have the right place.
                if ( id > data[middle-1].key() ) return middle;
                else {
                    top = middle - 1;
                }
            }
            else {
                bottom = middle + 1;
            }
        }
        /* Shouldn't be reached */
        return -1;
    }

    int binary_search_find(keyT id) const
    {
        int bottom = 0;
        int top = data.size() - 1;
        int middle;
        
        if ( data.size() == 0 ) return -1;
        while (bottom <= top) {
            middle = bottom + ( top - bottom ) / 2;
            if ( id == data[middle].key() ) return middle;
            else if ( id < data[middle].key() ) top = middle - 1;
            else bottom = middle + 1;
        }
        return -1;
    }

    friend class ConfigGraph;
    
public:
    typedef typename std::vector<classT>::iterator iterator;
    typedef typename std::vector<classT>::const_iterator const_iterator;
    
    // Essentially insert with a hint to look at end first.  This is
    // just here for backward compatibility for now.  Will be replaced
    // with insert() once things stabilize.
    void push_back(const classT& val)
    {
        // First look to see if it goes on the end.  If not, then find
        // where it goes.
        if ( data.size() == 0 ) {
            data.push_back(val);
            return;
        }
        if ( val.key() > data[data.size()-1].key() ) {
            data.push_back(val);
            return;
        }
        
        // Didn't belong at end, call regular insert
        insert(val);   
    }
    
    void insert(const classT& val)
    {
        int index = binary_search_insert(val.key());
        if ( index == -1 ) return;  // already in the map
        iterator it = data.begin();
        it += index;
        data.insert(it, val);
    }
    
    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }

    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }

    bool contains(keyT id) const
    {
        if ( binary_search_find(id) == -1 ) return false;
        return true;
    }
    
    classT& operator[] (keyT id)
    {
        int index = binary_search_find(id);
        if ( index == -1 ) {
            // Need to error out
        }
        return data[index]; 
    }
    
    const classT& operator[] (keyT id) const
    {
        int index = binary_search_find(id);
        if ( index == -1 ) {
            // Need to error out
        }
        return data[index];
    }
    
    void clear() { data.clear(); }
    size_t size() { return data.size(); }

};

template <typename keyT>
class SparseVectorMap<keyT,keyT> {
private:
    friend class SST::Core::Serialization::serialize<SparseVectorMap<keyT,keyT> >;

    std::vector<keyT> data;
    int binary_search_insert(keyT id) const
    {
        // For insert, we've found the right place when id < n && id >
        // n-1.  We then insert at n.

        int size = data.size();

        // Check to see if it goes top or bottom
        if ( size == 0 ) return 0;
        if ( id < data[0] ) return 0;
        if ( id > data[size-1] ) return size;
        
        int bottom = 0;
        int top = size - 1;
        int middle;

        while ( bottom <= top ) {
            middle = bottom + (top - bottom) / 2;
            if ( id == data[middle] ) return -1;  // Already in map
            if ( id < data[middle] ) {
                // May be the right place, need to see if id is greater
                // than the one before the current middle.  If so, then we
                // have the right place.
                if ( id > data[middle-1] ) return middle;
                else {
                    top = middle - 1;
                }
            }
            else {
                bottom = middle + 1;
            }
        }
        /* Shouldn't be reached */
        return -1;
    }
    
    int binary_search_find(keyT id) const
    {
        int bottom = 0;
        int top = data.size() - 1;
        int middle;
        
        if ( data.size() == 0 ) return -1;
        while (bottom <= top) {
            middle = bottom + ( top - bottom ) / 2;
            if ( id == data[middle] ) return middle;
            else if ( id < data[middle] ) top = middle - 1;
            else bottom = middle + 1;
        }
        return -1;
    }

    friend class ConfigGraph;
    
public:
    typedef typename std::vector<keyT>::iterator iterator;
    typedef typename std::vector<keyT>::const_iterator const_iterator;
    
    // Essentially insert with a hint to look at end first.  This is
    // just here for backward compatibility for now.  Will be replaced
    // with insert() once things stabilize.
    void push_back(const keyT& val)
    {
        // First look to see if it goes on the end.  If not, then find
        // where it goes.
        if ( data.size() == 0 ) {
            data.push_back(val);
            return;
        }
        if ( val.key() > data[data.size()-1].key() ) {
            data.push_back(val);
            return;
        }

        // Didn't belong at end, call regular insert
        insert(val);   
    }
    
    void insert(const keyT& val)
    {
        int index = binary_search_insert(val);
        if ( index == -1 ) return;  // already in the map
        iterator it = data.begin();
        it += index;
        data.insert(it, val);
    }
    
    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }

    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }

    bool contains(keyT id)
    {
        if ( binary_search_find(id) == -1 ) return false;
        return true;
    }
    
    keyT& operator[] (keyT id)
    {
        int index = binary_search_find(id);
        if ( index == -1 ) {
            // Need to error out
        }
        return data[index]; 
    }
    
    const keyT& operator[] (keyT id) const
    {
        int index = binary_search_find(id);
        if ( index == -1 ) {
            // Need to error out
        }
        return data[index];
    }
    
    void clear() { data.clear(); }
    size_t size() { return data.size(); }
    
};


 
} // namespace SST

namespace SST {
namespace Core {
namespace Serialization {

template <typename keyT, typename classT>
class serialize <SST::SparseVectorMap<keyT,classT>> {
public:
    void
        operator()(SST::SparseVectorMap<keyT,classT>& v, SST::Core::Serialization::serializer& ser) {
        ser & v.data;
    }
};
}
}
}


#endif // SST_CORE_CONFIGGRAPH_H
