// -*- c++ -*-

// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SPARSEVECTORMAP_H
#define SST_CORE_SPARSEVECTORMAP_H

#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"

#include <algorithm>
#include <string>
#include <vector>

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
class SparseVectorMap
{
private:
    friend class SST::Core::Serialization::serialize_impl<SparseVectorMap<keyT, classT>>;

    /**
       Finds the insertion point for new data

       @param id ID of the data that needs to be inserted.

       @return Index where new data should be inserted.  If key is
       already in the map, it will return -1.
     */
    std::vector<classT> data;
    int                 binary_search_insert(keyT id) const
    {
        // For insert, we've found the right place when id < n && id >
        // n-1.  We then insert at n.

        int size = data.size();

        // Check to see if it goes top or bottom
        if ( size == 0 ) return 0;
        if ( id > data[size - 1].key() ) return size;
        if ( id < data[0].key() ) return 0;

        int bottom = 0;
        int top    = size - 1;
        int middle;

        while ( bottom <= top ) {
            middle = bottom + (top - bottom) / 2;
            if ( id == data[middle].key() ) return -1; // Already in map
            if ( id < data[middle].key() ) {
                // May be the right place, need to see if id is greater
                // than the one before the current middle.  If so, then we
                // have the right place.
                if ( id > data[middle - 1].key() )
                    return middle;
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
        int top    = data.size() - 1;
        int middle;

        if ( data.size() == 0 ) return -1;
        while ( bottom <= top ) {
            middle = bottom + (top - bottom) / 2;
            if ( id == data[middle].key() )
                return middle;
            else if ( id < data[middle].key() )
                top = middle - 1;
            else
                bottom = middle + 1;
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
        if ( !sorted ) {
            std::sort(data.begin(), data.end,
                [](const classT& lhs, const classT& rhs) -> bool { return lhs.key() < rhs.key(); });
        }
    }

    using iterator       = typename std::vector<classT>::iterator;
    using const_iterator = typename std::vector<classT>::const_iterator;

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
        if ( index == -1 ) return data[binary_search_find(val.key())]; // already in the map
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
    classT& operator[](keyT id)
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
    const classT& operator[](keyT id) const
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
   Exception that is thrown by SparseVectorMap::filter() if the
   filtering object returns an object that returns a different value
   from the key() function than what was passed into the filter.
 */
class bad_filtered_key_error : public std::runtime_error
{
public:
    explicit bad_filtered_key_error(const std::string& what_arg) :
        runtime_error(what_arg)
    {}
};


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
template <typename keyT, typename classT>
class SparseVectorMap<keyT, classT*>
{
private:
    friend class SST::Core::Serialization::serialize_impl<SparseVectorMap<keyT, classT*>>;

    /**
       Finds the insertion point for new data

       @param id ID of the data that needs to be inserted.

       @return Index where new data should be inserted.  If key is
       already in the map, it will return -1.
     */
    std::vector<classT*> data;
    int                  binary_search_insert(keyT id) const
    {
        // For insert, we've found the right place when id < n && id >
        // n-1.  We then insert at n.

        int size = data.size();

        // Check to see if it goes top or bottom
        if ( size == 0 ) return 0;
        if ( id > data[size - 1]->key() ) return size;
        if ( id < data[0]->key() ) return 0;

        int bottom = 0;
        int top    = size - 1;
        int middle;

        while ( bottom <= top ) {
            middle = bottom + (top - bottom) / 2;
            if ( id == data[middle]->key() ) return -1; // Already in map
            if ( id < data[middle]->key() ) {
                // May be the right place, need to see if id is greater
                // than the one before the current middle.  If so, then we
                // have the right place.
                if ( id > data[middle - 1]->key() )
                    return middle;
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
        int top    = data.size() - 1;
        int middle;

        if ( data.size() == 0 ) return -1;
        while ( bottom <= top ) {
            middle = bottom + (top - bottom) / 2;
            if ( id == data[middle]->key() )
                return middle;
            else if ( id < data[middle]->key() )
                top = middle - 1;
            else
                bottom = middle + 1;
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
    SparseVectorMap(std::vector<classT*>& new_data, bool sorted = false)
    {
        data.swap(new_data);
        if ( !sorted ) {
            std::sort(data.begin(), data.end,
                [](const classT* lhs, const classT* rhs) -> bool { return lhs->key() < rhs->key(); });
        }
    }

    using iterator       = typename std::vector<classT*>::iterator;
    using const_iterator = typename std::vector<classT*>::const_iterator;

    /**
       Insert new value into SparseVectorMap.  The inserted class must
       have a key() function with return type keyT.

       @param val value to add to SparseVectorMap

       @return reference to the inserted item, or to the existing item
       if it was already present in the map.

    */
    classT* insert(classT* val)
    {
        int index = binary_search_insert(val->key());
        if ( index == -1 ) return data[binary_search_find(val->key())]; // already in the map
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
    classT* operator[](keyT id)
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
    const classT* operator[](keyT id) const
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


    /**
       Function to filter the contents of the SparseVectorMap.  Takes
       an object with an overloaded operator() function that takes as
       argument the current item.  The funtion returns the object that
       should take the place of this object (value returned by key()
       function must be same for both objects), or returns nullptr if
       the object should be deleted.  When an item is deleted, the
       size of the map reduces by 1.

       @param filt Filter object to use for filtering contents of map

       @exception bad_filtered_key_error filter returned an object that
       didn't return the same value on a call to key() as the original
       object in the map.
    */
    template <typename filterT>
    void filter(filterT& filt)
    {
        int write_ptr = 0;
        for ( size_t i = 0; i < data.size(); ++i ) {
            auto key        = data[i]->key();
            data[write_ptr] = filt(data[i]);
            if ( data[write_ptr] != nullptr ) {
                // Make sure the keys match
                if ( UNLIKELY(data[write_ptr]->key() != key) ) {
                    // No recovering from this, just need to error out
                    throw bad_filtered_key_error(
                        "ERROR: Filter object passed to SparseVectorMap::filter returned an object that had a "
                        "different value of key() than what was passed into the filter.  The filter object must return "
                        "either an object with the same value of key(), or nullptr if the object should be removed.");
                }
                write_ptr++;
            }
        }
        // Need to shorten vector if items were removed.  The
        // write_ptr will be at the index with the first nullptr from
        // the filter, and tells us where to cut things off.  So,
        // simply call resize() on the vector with write_ptr as the
        // new size.
        data.resize(write_ptr);
        data.shrink_to_fit();
    }
};

/**
   Templated version of SparseVectorMap where the data and key are the
   same (actually more like a set than a map in this case).  The type
   must implement the less than operator.  This is primarily intended
   for use with native types.
 */
template <typename keyT>
class SparseVectorMap<keyT, keyT>
{
private:
    friend class SST::Core::Serialization::serialize_impl<SparseVectorMap<keyT, keyT>>;

    /**
       Finds the insertion point for new data

       @param id ID of the data that needs to be inserted.

       @return Index where new data should be inserted.  If key is
       already in the map, it will return -1.
     */
    std::vector<keyT> data;
    int               binary_search_insert(keyT id) const
    {
        // For insert, we've found the right place when id < n && id >
        // n-1.  We then insert at n.

        int size = data.size();

        // Check to see if it goes top or bottom
        if ( size == 0 ) return 0;
        if ( id < data[0] ) return 0;
        if ( id > data[size - 1] ) return size;

        int bottom = 0;
        int top    = size - 1;
        int middle;

        while ( bottom <= top ) {
            middle = bottom + (top - bottom) / 2;
            if ( id == data[middle] ) return -1; // Already in map
            if ( id < data[middle] ) {
                // May be the right place, need to see if id is greater
                // than the one before the current middle.  If so, then we
                // have the right place.
                if ( id > data[middle - 1] )
                    return middle;
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
        int top    = data.size() - 1;
        int middle;

        if ( data.size() == 0 ) return -1;
        while ( bottom <= top ) {
            middle = bottom + (top - bottom) / 2;
            if ( id == data[middle] )
                return middle;
            else if ( id < data[middle] )
                top = middle - 1;
            else
                bottom = middle + 1;
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
        if ( !sorted ) {
            std::sort(data.begin(), data.end, [](const keyT& lhs, const keyT& rhs) -> bool { return lhs < rhs; });
        }
    }

    using iterator       = typename std::vector<keyT>::iterator;
    using const_iterator = typename std::vector<keyT>::const_iterator;

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
        if ( index == -1 ) return data[binary_search_find(val)]; // already in the map
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
    keyT& operator[](keyT id)
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
    const keyT& operator[](keyT id) const
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

namespace SST::Core::Serialization {

template <typename keyT, typename classT>
class serialize_impl<SST::SparseVectorMap<keyT, classT>>
{
public:
    void operator()(SST::SparseVectorMap<keyT, classT>& v, SST::Core::Serialization::serializer& ser, ser_opt_t options)
    {
        SST_SER(v.data, options);
    }
};
} // namespace SST::Core::Serialization

#endif // SST_CORE_SPARSEVECTORMAP_H
