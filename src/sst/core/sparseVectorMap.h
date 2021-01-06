// -*- c++ -*-

// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SPARSEVECTORMAP_H
#define SST_CORE_SPARSEVECTORMAP_H

#include "sst/core/sst_types.h"
#include "sst/core/serialization/serializable.h"

#include <vector>
#include <algorithm>

namespace SST {

/**
   Class that stores data in a vector, but can access the data similar
   to a map.  The data structure is O(log n) on reads, but is O(n) to
   insert.  The primary use case is when data is inserted in order, but
   accessed randomly.  You can also create the SparseVectorMap with a
   vector already loaded with the data.  If the data is not already
   sorted, it will call std::sort on the data, which likely has an
   average complexity of O(n log n).  This data structure should not
   be used for large lists where inserts do not happen in sorted order.

   NOTE: Since the data is stored in the vector, reference returned
   from the various accessor functions will not be valid longterm.  If
   an insert causes the vector to be resized, all references returned
   before that reallocation may (likely will) be invalid.  References
   are only guaranteed to be valid until the next write to the data
   structure.
 */
template <typename keyT, typename classT = keyT>
class SparseVectorMap {
private:
    friend class SST::Core::Serialization::serialize<SparseVectorMap<keyT,classT> >;

    /**
       Finds the insertion point for new data

       @param id ID of the data that needs to be inserted.

       @return Index where new data should be inserted.  If key is
       already in the map, it will return -1.
     */
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

    /**
       Finds the index where the data associated with the given key is
       found in the vector

       @param id ID of the data that needs to be found.

       @return Index where data for the provided ID is found.  It
       returns -1 if the key is not found in the data vector.
     */
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

    /**
       Default constructor for SparseVectorMap
     */
    SparseVectorMap() {}

    /**
       Constructor that allows you to pass an already filled in array
       with data.  The data in the passed in vector will be swapped
       into the data vector of the sparsevectormap and the passed in
       vector will be empty.

       @param new_data Vector of data to swap into the sparsevectormap
       data

       @param sorted Specifies whether the vector is already sorted in
       ascending order. if not, it will be sorted after swapping the
       data in.
    */
    SparseVectorMap(std::vector<classT>& new_data, bool sorted = false)
    {
        data.swap(new_data);
        if (!sorted) {
            std::sort(data.begin(),data.end,
                [] (const classT& lhs, const classT& rhs) -> bool
                    {
                        return lhs.key() < rhs.key();
                    });
        }
    }

    typedef typename std::vector<classT>::iterator iterator;
    typedef typename std::vector<classT>::const_iterator const_iterator;

    // Essentially insert with a hint to look at end first.  this is
    // just here for backward compatibility for now.  Will be replaced
    // with insert() once things stabilize.
    classT& push_back(const classT& val) __attribute__ ((deprecated("SparseVectorMap::push_back() is deprecated and will be removed in sst 12. please simply use sparsevectormap::insert.")))
    {
        // first look to see if it goes on the end.  if not, then find
        // where it goes.
        if ( data.size() == 0 ) {
            data.push_back(val);
            return data.back();
        }
        if ( val.key() > data[data.size()-1].key() ) {
            data.push_back(val);
            return data.back();
        }

        // didn't belong at end, call regular insert
        return insert(val);
    }

    /**
       Insert new value into SparseVectorMap.  The inserted class must
       have a key() function with return type keyT.

       @param val value to add to SparseVectorMap

       @return reference to the inserted item, or to the existing item
       if it was already present in the map.

    */
   classT& insert(const classT& val)
    {
        int index = binary_search_insert(val.key());
        if ( index == -1 ) return data[binary_search_find(val.key())];  // already in the map
        iterator it = data.begin();
        it += index;
        data.insert(it, val);
        return data[index];
    }

    /**
       Returns the begin iterator to the underlying vector.

       @return begin iterator to data vector
     */
    iterator begin() { return data.begin(); }

    /**
       Returns the end iterator to the underlying vector.

       @return end iterator to data vector
     */
    iterator end() { return data.end(); }

    /**
       Returns the const begin iterator to the underlying vector.

       @return const begin iterator to data vector
     */
    const_iterator begin() const { return data.begin(); }

    /**
       Returns the const end iterator to the underlying vector.

       @return const end iterator to data vector
     */
    const_iterator end() const { return data.end(); }

    /**
       Checks if the provided id is found in the SparseVectorMap

       @param id id to check for

       @return true if id is found, false otherwise
     */
    bool contains(keyT id) const
    {
        if ( binary_search_find(id) == -1 ) return false;
        return true;
    }

    /**
       Operator returns a reference to data with the specified id.
       Value can be modified.  This will only return references to
       existing values, you must use insert() for new values.

       @param id id of the value to return (value returned by key())

       @return reference to the requested item.

    */
    classT& operator[] (keyT id)
    {
        int index = binary_search_find(id);
        if ( index == -1 ) {
            // Need to error out
        }
        return data[index];
    }

    /**
       Operator returns a const reference to data with the specified
       id.  Value cannot be modified.  This will only return
       references to existing values, you must use insert() for new
       values.

       @param id id of the value to return (value returned by key())

       @return const reference to the requested item.

    */
    const classT& operator[] (keyT id) const
    {
        int index = binary_search_find(id);
        if ( index == -1 ) {
            // Need to error out
        }
        return data[index];
    }

    /**
       Clears the contents of the SparseVectorMap
     */
    void clear() { data.clear(); }

    /**
       Returns the number of items in the SparseVectorMap

       @return number of items
     */
    size_t size() { return data.size(); }

};


/**
   Templated version of SparseVectorMap where the data and key are the
   same (actually more like a set than a map in this case).  The type
   must implement the less than operator.  This is primarily intended
   for use with native types.
 */
template <typename keyT>
class SparseVectorMap<keyT,keyT> {
private:
    friend class SST::Core::Serialization::serialize<SparseVectorMap<keyT,keyT> >;

    /**
       Finds the insertion point for new data

       @param id ID of the data that needs to be inserted.

       @return Index where new data should be inserted.  If key is
       already in the map, it will return -1.
     */
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

    /**
       Finds the index where the data associated with the given key is
       found in the vector

       @param id ID of the data that needs to be found.

       @return Index where data for the provided ID is found.  It
       returns -1 if the key is not found in the data vector.
     */
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

    /**
       Default constructor for SparseVectorMap
     */
    SparseVectorMap() {}

    /**
       Constructor that allows you to pass an already filled in array
       with data.  The data in the passed in vector will be swapped
       into the data vector of the SparseVectorMap and the passed in
       vector will be empty.

       @param new_data Vector of data to swap into the SparseVectorMap
       data

       @param sorted Specifies whether the vector is already sorted in
       ascending order. If not, it will be sorted after swapping the
       data in.
    */
    SparseVectorMap(std::vector<keyT>& new_data, bool sorted = false)
    {
        data.swap(new_data);
        if (!sorted) {
            std::sort(data.begin(),data.end,
                [] (const keyT& lhs, const keyT& rhs) -> bool
                    {
                        return lhs < rhs;
                    });
        }
    }


    typedef typename std::vector<keyT>::iterator iterator;
    typedef typename std::vector<keyT>::const_iterator const_iterator;

    // Essentially insert with a hint to look at end first.  This is
    // just here for backward compatibility for now.  Will be replaced
    // with insert() once things stabilize.
    keyT& push_back(const keyT& val) __attribute__ ((deprecated("SparseVectorMap::push_back is deprecated and will be removed in SST 12. Please simply use SparseVectorMap::insert.")))
    {
        // First look to see if it goes on the end.  If not, then find
        // where it goes.
        if ( data.size() == 0 ) {
            data.push_back(val);
            return data.back();
        }
        if ( val.key() > data[data.size()-1].key() ) {
            data.push_back(val);
            return data.back();
        }

        // Didn't belong at end, call regular insert
        return insert(val);
    }

    /**
       Insert new value into SparseVectorMap.  The inserted class must
       have a key() function with return type keyT.

       @param val value to add to SparseVectorMap

       @return reference to the inserted item, or to the existing item
       if it was already present in the map.

    */
    keyT& insert(const keyT& val)
    {
        int index = binary_search_insert(val);
        if ( index == -1 ) return data[binary_search_find(val)];  // already in the map
        iterator it = data.begin();
        it += index;
        data.insert(it, val);
        return data[index];
    }

    /**
       Returns the begin iterator to the underlying vector.

       @return begin iterator to data vector
     */
    iterator begin() { return data.begin(); }

    /**
       Returns the end iterator to the underlying vector.

       @return end iterator to data vector
     */
    iterator end() { return data.end(); }

    /**
       Returns the const begin iterator to the underlying vector.

       @return const begin iterator to data vector
     */
    const_iterator begin() const { return data.begin(); }

    /**
       Returns the const end iterator to the underlying vector.

       @return const end iterator to data vector
     */
    const_iterator end() const { return data.end(); }

    /**
       Checks if the provided id is found in the SparseVectorMap

       @param id id to check for

       @return true if id is found, false otherwise
     */
    bool contains(keyT id)
    {
        if ( binary_search_find(id) == -1 ) return false;
        return true;
    }

    /**
       Operator returns a reference to data with the specified id.
       Value can be modified.  This will only return references to
       existing values, you must use insert() for new values.

       @param id id of the value to return (value returned by key())

       @return reference to the requested item.

    */
    keyT& operator[] (keyT id)
    {
        int index = binary_search_find(id);
        if ( index == -1 ) {
            // Need to error out
        }
        return data[index];
    }

    /**
       Operator returns a const reference to data with the specified
       id.  Value cannot be modified.  This will only return
       references to existing values, you must use insert() for new
       values.

       @param id id of the value to return (value returned by key())

       @return const reference to the requested item.

    */
    const keyT& operator[] (keyT id) const
    {
        int index = binary_search_find(id);
        if ( index == -1 ) {
            // Need to error out
        }
        return data[index];
    }

    /**
       Clears the contents of the SparseVectorMap
     */
    void clear() { data.clear(); }

    /**
       Returns the number of items in the SparseVectorMap

       @return number of items
     */
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
