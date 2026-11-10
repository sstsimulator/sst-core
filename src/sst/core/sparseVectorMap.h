// -*- c++ -*-

// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
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
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace SST {

/**
   Exception that is thrown by SparseVectorMap::filter() if the
   filtering object returns an object that returns a different value
   from the key() function than what was passed into the filter --
   i.e. the filterT functor passed to filter() is implemented
   incorrectly.

   This is unrecoverable: filter() performs its element replacement
   in place as it scans, so detecting the bad key happens only after
   part of the container has already been overwritten with the
   filter's output and the element that triggered the error (along
   with everything after it that hadn't been scanned yet) is gone.
   There is no way to reconstruct what should have been there. Once
   this is thrown, the SparseVectorMap it came from has lost data and
   must not be used further -- treat it as a fatal programming error
   in the filterT implementation, not a condition to catch and
   continue past.
 */
class bad_filtered_key_error : public std::runtime_error
{
public:
    explicit bad_filtered_key_error(const std::string& what_arg) :
        runtime_error(what_arg)
    {}
};

namespace SparseVectorMapDetail {

// Default key-extraction policy, and the model for a custom one (see
// SparseVectorMap's KeyExtract template parameter). Extracts the
// ordering key from a stored element. SparseVectorMap supports four
// element shapes out of the box (plain value with a key() method,
// pointer to such a value, a value that IS its own key, or a
// std::pair<keyT, V> keyed by its first member), and this is the one
// place that distinguishes them, instead of hand-duplicated container
// implementations.
//
// A custom KeyExtract need only provide a static get() callable the
// same way: KeyExtract::get(item) taking something classT converts
// to, and returning something convertible to keyT.
// The key() forms return whatever key() returns, so a key() returning a
// const reference isn't copied on every comparison.
template <typename keyT, typename classT>
struct KeyOf
{
    static decltype(auto) get(const classT& item) { return item.key(); }
};

template <typename keyT, typename classT>
struct KeyOf<keyT, classT*>
{
    static decltype(auto) get(const classT* item) { return item->key(); }
};

template <typename keyT>
struct KeyOf<keyT, keyT>
{
    static const keyT& get(const keyT& item) { return item; }
};

// std::pair<keyT, V>: keyed by first.  Only an exact match on the key
// type, so a set of pairs (keyT == classT == std::pair<A, B>) still
// uses the identity form above instead of being ambiguous with it.
template <typename keyT, typename V>
struct KeyOf<keyT, std::pair<keyT, V>>
{
    static const keyT& get(const std::pair<keyT, V>& item) { return item.first; }
};

// std::pair<const keyT, V> (std::map's value_type) cannot be stored:
// sort() and insert() need assignable elements.
template <typename keyT, typename V>
struct KeyOf<keyT, std::pair<const keyT, V>>
{
    static_assert(!std::is_same<V, V>::value,
        "SparseVectorMap cannot store std::pair<const keyT, V> because its elements must be assignable; use "
        "std::pair<keyT, V> instead.");
    static const keyT& get(const std::pair<const keyT, V>& item) { return item.first; }
};

// Enables SparseVectorMap's pair-only value methods (value(),
// findValue(), insert_or_assign() and the two-argument insert()).  They
// build and look up elements by passing the key separately from the
// value, which is only right when the key is first, so they require the
// built-in pair KeyOf, not a custom KeyExtract.
template <typename keyT, typename classT, typename KeyExtract>
struct PairValue
{
    static constexpr bool enabled = false;
};

template <typename keyT, typename V>
struct PairValue<keyT, std::pair<keyT, V>, KeyOf<keyT, std::pair<keyT, V>>>
{
    static constexpr bool enabled = true;
    using type                    = V;
};

} // namespace SparseVectorMapDetail

/**
   Class that stores data in a vector, but can access the data similar
   to a map.  Lookups are O(log n).

   Internally the vector holds two sorted runs: a main sorted region
   followed by a small sorted "tail" of keys that were inserted out of
   order.  An insert whose key is larger than every key already present
   (with no tail pending) is simply appended, O(1) amortized.  Any
   other insert is placed in its sorted position within the tail,
   which only moves tail elements.  Once the tail reaches a size limit
   it is merged into the main region with std::inplace_merge.  The
   limit is max(64, 2*sqrt(size())), computed whenever the map becomes
   fully sorted (after a merge, sort() or construction from a vector).
   Below that same 64-element floor, no tail is ever opened in the
   first place: every insert goes directly to its sorted position
   across the single run, so small maps -- the common case for things
   like a component's parameter set -- are always a single sorted
   run, and contains()/operator[] never pay for a tail's second binary
   search.

   Performance summary, from benchmarks against std::map and std::set
   (ratios are SparseVectorMap time / std::map time):

   Efficient:

     - Inserting keys in increasing order, with any interleaving of
       reads: every insert is an append.  About 2-5x faster than
       std::map from a few hundred elements up (1M tested).  This is
       the intended use, and how every user in SST core inserts.

     - Building from a vector with the
       SparseVectorMap(std::vector<classT>&, bool) constructor: one
       O(n log n) sort.  Faster than inserting the same keys into a
       std::map even in random order (about 1.3-1.8x up to 16k
       elements, rising to 4.5x at 1M).

     - Lookups with cheap keys (integers).  About even with std::map
       up to ~16k elements, 1.1-1.4x faster at 64k and 2.2-3.3x faster
       at 256k-1M, because a flat sorted array is far more cache
       friendly than a tree.

     - Small maps with cheap keys, even in random order: at 16-64
       elements with uint32_t keys, inserts and lookups are within a
       few percent of std::map or faster.

     - Memory: about 33% below std::map for elements holding a
       std::string, and 35-74% below for pointer, small POD and
       set-like elements.

   Not efficient:

     - Inserting keys out of order one at a time past a few hundred
       elements.  Once a tail exists each insert costs O(sqrt(n))
       amortized element moves, versus O(log n) for std::map: about
       1.3x slower at 1k elements, 1.6x at 16k and 2x at 64k-1M with
       uint32_t keys and std::string values, and worse for larger
       elements (3.3x at 500k for a std::string-holding value type).
       Collect such keys in a vector and use the vector constructor,
       or use std::map if they must be inserted one at a time.

     - Random-order inserts with expensive-to-compare keys such as
       std::string, even at small sizes: about 1.1-1.7x slower than
       std::map at 16-64 elements.  Lookups with such keys are within
       about 10% of std::map at those sizes.  Larger string-key maps
       have not been measured.

   Interleaving reads with inserts adds no extra cost at any size.

   contains() and operator[]() search both runs (two binary searches
   when a tail is pending, one otherwise) and never reorder storage.

   Reference and iterator validity:

     - insert(), erase(), filter(), sort(), compact() and clear() may
       invalidate all references and iterators into the map.

     - begin(), end() and find() (const and non-const) first merge any
       pending tail into the main region so they can return iterators
       over fully sorted data.  Calling them MAY ALSO INVALIDATE all
       references and iterators previously obtained from the map.
       Because the const overloads can reorganize internal storage,
       they are not safe to call concurrently with any other access to
       the same map -- unless compact() has been called since the last
       insert(), erase(), sort(), filter() or clear(), in which case
       there is nothing left to merge and they only read.

     - contains() and operator[]() never invalidate references or
       iterators.

   classT may be a plain type with a key() method returning keyT, a
   pointer to such a type (SparseVectorMap does not take ownership of
   the pointee), identical to keyT itself (set-like usage, for native
   types with no key() method), or a std::pair<keyT, V> keyed by its
   first member:

     SparseVectorMap<uint32_t, std::pair<uint32_t, std::string>> map;
     map.insert({ 7, "seven" });
     const std::string& s = map[7].second;

   For pair elements, the usual accessors (insert(), operator[](),
   find(), iteration) return the whole pair, as std::map's iterators
   do.  Additional methods work on second only:

     map.insert_or_assign(7, "SEVEN");      // insert, or replace the value
     map.insert(8, "eight");                // insert; leaves an existing value
     std::string& v = map.value(7);         // like operator[]: throws if absent
     std::string& w = map.at(7);            // same as value(), like std::map::at()
     const std::string* p = map.findValue(9);  // nullptr if absent

   findValue() never modifies the map, so it is safe for concurrent
   const access even when a tail is pending.  These methods require the
   default KeyExtract, since they assume the key is first.  The pair's
   first type must be exactly keyT.  Do not modify first through a
   returned reference: like changing a key() field, that silently
   breaks the sort order until sort() is called.  std::pair<const keyT, V>, the value_type
   of std::map, is rejected at compile time because its elements
   cannot be assigned; use std::pair<keyT, V>.

   For a classT that doesn't have a key() method at all -- or where
   the key you want isn't what key() would return -- supply a fourth
   template argument, KeyExtract: a type with a static get() callable
   as KeyExtract::get(item), returning something convertible to keyT.
   This bypasses the key()/->key()/identity/pair auto-detection entirely.
   For example, a legacy record type with no key() method:

     struct LegacyRecord { uint64_t recordId; std::string payload; };
     struct LegacyRecordKey { static uint64_t get(const LegacyRecord& r) { return r.recordId; } };
     SparseVectorMap<uint64_t, LegacyRecord, LegacyRecordKey> map;

   KeyExtract defaults to SparseVectorMapDetail::KeyOf<keyT, classT>
   (the key()/->key()/identity/pair auto-detection above), so this is
   purely additive -- no existing instantiation is affected.
 */
template <typename keyT, typename classT = keyT, typename KeyExtract = SparseVectorMapDetail::KeyOf<keyT, classT>>
class SparseVectorMap
{
private:
    friend class SST::Core::Serialization::serialize_impl<SparseVectorMap<keyT, classT, KeyExtract>>;

    static_assert(std::is_convertible<decltype(KeyExtract::get(std::declval<const classT&>())), keyT>::value,
        "SparseVectorMap's KeyExtract type must provide a static get() method callable as "
        "KeyExtract::get(const classT&) (or anything classT converts to), returning something "
        "convertible to keyT.");

    // Returns a reference when KeyExtract::get() returns a reference to a
    // keyT (the identity and pair forms do), so comparisons don't copy
    // keys that are expensive to copy, such as std::string.  Otherwise
    // returns a keyT by value, so a get() returning some other type is
    // still converted to keyT before it is compared.
    using KeyGetResult = decltype(KeyExtract::get(std::declval<const classT&>()));
    using KeyRef       = typename std::conditional<std::is_reference<KeyGetResult>::value &&
                                                       std::is_same<typename std::decay<KeyGetResult>::type, keyT>::value,
              const keyT&, keyT>::type;

    static KeyRef getKey(const classT& item) { return KeyExtract::get(item); }

    // A function object rather than a function, so std::sort and
    // std::inplace_merge can inline the comparison; passing a function
    // pointer made sorting about 1.3-2x slower.
    struct KeyLess
    {
        bool operator()(const classT& lhs, const classT& rhs) const { return getKey(lhs) < getKey(rhs); }
    };

    // mutable only so the const overloads of begin()/end()/find() can
    // merge a pending tail; contains() and operator[] never modify these.
    mutable std::vector<classT> data;

    // data[0, boundary_) and data[boundary_, data.size()) are each
    // sorted.  The map is fully sorted iff boundary_ == data.size().
    mutable size_t boundary_ = 0;

    // Tail size at which insert() merges the tail into the main region,
    // and also the smallest map size at which a tail is ever opened in
    // the first place (see insert()): below it, every insert goes
    // directly to its sorted position across the single run, so
    // contains()/operator[] never pay for the tail's second binary
    // search.  Shifting costs the same either way at these sizes, so
    // this only helps.  Once a tail does exist, shifting within it
    // costs ~n*T/2 moves over n out-of-order inserts and merging costs
    // ~n^2/T, so the optimum grows like sqrt(n); max(64, 2*sqrt(size))
    // was within a few percent of the best fixed value at every size
    // (20k-500k) and element type tested.
    static constexpr size_t kMinTailSize = 64;

    // Cached tail size limit, recomputed only when the map becomes fully
    // sorted, so out-of-order inserts never pay for a sqrt.
    mutable size_t tailLimit_ = kMinTailSize;

    void updateTailLimit() const
    {
        tailLimit_ =
            std::max<size_t>(kMinTailSize, static_cast<size_t>(2.0 * std::sqrt(static_cast<double>(data.size()))));
    }

    static constexpr size_t npos = static_cast<size_t>(-1);

    // id is taken by const reference throughout this chain (and in the
    // public lookup methods below) rather than by value: keyT is often
    // cheap to copy (int, uint32_t) but need not be (e.g. std::string),
    // and a by-value id would be copied again at every hop.
    size_t lowerBound(const keyT& id, size_t lo, size_t hi) const
    {
        auto it = std::lower_bound(data.begin() + lo, data.begin() + hi, id,
            [](const classT& item, const keyT& k) { return getKey(item) < k; });
        return it - data.begin();
    }

    size_t findIn(const keyT& id, size_t lo, size_t hi) const
    {
        size_t pos = lowerBound(id, lo, hi);
        if ( pos != hi && getKey(data[pos]) == id ) return pos;
        return npos;
    }

    // Searches both runs without modifying anything.
    size_t findIndex(const keyT& id) const
    {
        size_t pos = findIn(id, 0, boundary_);
        if ( pos != npos || boundary_ == data.size() ) return pos;
        if ( id < getKey(data[boundary_]) || getKey(data.back()) < id ) return npos;
        return findIn(id, boundary_, data.size());
    }

    void mergeTail() const
    {
        if ( boundary_ == data.size() ) return;
        std::inplace_merge(data.begin(), data.begin() + boundary_, data.end(), KeyLess());
        boundary_ = data.size();
        updateTailLimit();
    }

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

       This is the fastest way to build a map from data that arrives
       in random order: one O(n log n) sort instead of one insert() per
       element.

       The keys in new_data must be unique.  Duplicates are not
       detected or removed; if present, lookups may return any one of
       the duplicate elements.

       @param new_data Vector of data to swap into the sparsevectormap
       data.  Every element must have a distinct key.

       @param sorted Specifies whether the vector is already sorted in
       ascending order. If not, it will be sorted after swapping the
       data in.
    */
    SparseVectorMap(std::vector<classT>& new_data, bool sorted = false)
    {
        data.swap(new_data);
        if ( !sorted ) std::sort(data.begin(), data.end(), KeyLess());
        boundary_ = data.size();
        updateTailLimit();
    }

    /**
       Force a full sort of the data.  Use this after mutating a stored
       element's key directly (e.g. through a reference) to restore the
       sorted invariant.  Invalidates all references and iterators.
     */
    void sort()
    {
        std::sort(data.begin(), data.end(), KeyLess());
        boundary_ = data.size();
        updateTailLimit();
    }

    /**
       Finish any pending reorganization and, optionally, release spare
       capacity.  Call this once you are done inserting.  Afterwards the
       const begin(), end() and find() never modify the map, so
       concurrent const access is safe until the next insert(), erase(),
       sort(), filter() or clear().  Inserting again afterwards is
       allowed; it just makes the work done here wasted.

       Lookups of keys that are present do not get measurably faster,
       but lookups of absent keys may (about 5-13% at 64k elements and
       up).  Releasing memory can save up to half of the vector's
       allocation, since vector growth may leave spare capacity.

       Invalidates all references and iterators.

       @param release_memory If true (the default), shrink the
       underlying vector's capacity to its size.
     */
    void compact(bool release_memory = true)
    {
        mergeTail();
        if ( release_memory ) data.shrink_to_fit();
    }

    using iterator       = typename std::vector<classT>::iterator;
    using const_iterator = typename std::vector<classT>::const_iterator;

    /**
       Insert new value into SparseVectorMap.  May invalidate all
       references and iterators into the map.

       @param val value to add to SparseVectorMap

       @return reference to the inserted item, or to the existing item
       if it was already present in the map.
    */
    classT& insert(const classT& val)
    {
        keyT key = getKey(val);

        if ( boundary_ == data.size() && (data.empty() || getKey(data.back()) < key) ) {
            data.push_back(val);
            ++boundary_;
            return data.back();
        }

        // Below kMinTailSize, never open a tail: insert directly into
        // its sorted position across the whole (already fully sorted)
        // vector instead.  Shifting costs the same as it would within
        // a tail -- at most kMinTailSize elements move either way --
        // but reads stay a single binary search instead of two plus a
        // range check, which is most of the cost at these sizes.  Once
        // a tail already exists (from crossing the threshold earlier,
        // or surviving a shrink via erase()), fall through and extend
        // it as before; two runs are always kept, never a third.
        if ( boundary_ == data.size() && data.size() < kMinTailSize ) {
            // One search gives both the duplicate check and the position
            size_t pos = lowerBound(key, 0, data.size());
            if ( pos != data.size() && getKey(data[pos]) == key ) return data[pos];
            data.insert(data.begin() + pos, val);
            boundary_ = data.size();
            return data[pos];
        }

        size_t existing = findIndex(key);
        if ( existing != npos ) return data[existing];

        size_t pos = lowerBound(key, boundary_, data.size());
        data.insert(data.begin() + pos, val);

        if ( data.size() - boundary_ >= tailLimit_ ) {
            mergeTail();
            return data[findIn(key, 0, data.size())];
        }
        return data[pos];
    }

    /**
       Returns the begin iterator to the underlying vector.  Merges any
       pending tail first, which MAY INVALIDATE all references and
       iterators previously obtained from the map.

       @return begin iterator to data vector
     */
    iterator begin()
    {
        mergeTail();
        return data.begin();
    }

    /**
       Returns the end iterator to the underlying vector.  Merges any
       pending tail first, which MAY INVALIDATE all references and
       iterators previously obtained from the map.

       @return end iterator to data vector
     */
    iterator end()
    {
        mergeTail();
        return data.end();
    }

    /**
       Returns the const begin iterator to the underlying vector.
       Merges any pending tail first, which MAY INVALIDATE all
       references and iterators previously obtained from the map.

       @return const begin iterator to data vector
     */
    const_iterator begin() const
    {
        mergeTail();
        return data.begin();
    }

    /**
       Returns the const end iterator to the underlying vector.  Merges
       any pending tail first, which MAY INVALIDATE all references and
       iterators previously obtained from the map.

       @return const end iterator to data vector
     */
    const_iterator end() const
    {
        mergeTail();
        return data.end();
    }

    /**
       Checks if the provided id is found in the SparseVectorMap.
       Never invalidates references or iterators.

       @param id id to check for

       @return true if id is found, false otherwise
     */
    bool contains(const keyT& id) const { return findIndex(id) != npos; }

    /**
       Operator returns a reference to data with the specified id.
       Value can be modified.  This will only return references to
       existing values, you must use insert() for new values.  Never
       invalidates references or iterators.

       @param id id of the value to return (value returned by key())

       @return reference to the requested item.

       @exception std::out_of_range try to access an element that doesn't exist in the SparseVectorMap
    */
    classT& operator[](const keyT& id)
    {
        size_t index = findIndex(id);
        if ( index == npos ) {
            throw std::out_of_range("SparseVectorMap: trying to access element that does not exist");
        }
        return data[index];
    }

    /**
       Operator returns a const reference to data with the specified
       id.  Value cannot be modified.  This will only return
       references to existing values, you must use insert() for new
       values.  Never invalidates references or iterators.

       @param id id of the value to return (value returned by key())

       @return const reference to the requested item.

       @exception std::out_of_range try to access an element that doesn't exist in the SparseVectorMap
    */
    const classT& operator[](const keyT& id) const
    {
        size_t index = findIndex(id);
        if ( index == npos ) {
            throw std::out_of_range("SparseVectorMap: trying to access element that does not exist");
        }
        return data[index];
    }

    // The methods below are only available for std::pair<keyT, V>
    // elements using the default KeyExtract (see the class comment).
    // They work on the pair's second member.

private:
    template <typename U>
    using EnableIfPair =
        typename std::enable_if<SparseVectorMapDetail::PairValue<keyT, U, KeyExtract>::enabled, int>::type;

    template <typename U>
    using PairValueT = typename SparseVectorMapDetail::PairValue<keyT, U, KeyExtract>::type;

public:

    /**
       Returns the value (second) of the element with the specified
       id.  Never invalidates references or iterators.

       @param id id of the element

       @return reference to the element's second member

       @exception std::out_of_range the id is not in the SparseVectorMap
     */
    template <typename U = classT, EnableIfPair<U> = 0>
    PairValueT<U>& value(const keyT& id)
    {
        return (*this)[id].second;
    }

    /**
       Returns the value (second) of the element with the specified
       id.  Never invalidates references or iterators.

       @param id id of the element

       @return const reference to the element's second member

       @exception std::out_of_range the id is not in the SparseVectorMap
     */
    template <typename U = classT, EnableIfPair<U> = 0>
    const PairValueT<U>& value(const keyT& id) const
    {
        return (*this)[id].second;
    }

    /**
       Same as value(), named and behaving like std::map::at(): returns
       the value (second) of the element with the specified id.  Never
       invalidates references or iterators.

       @param id id of the element

       @return reference to the element's second member

       @exception std::out_of_range the id is not in the SparseVectorMap
     */
    template <typename U = classT, EnableIfPair<U> = 0>
    PairValueT<U>& at(const keyT& id)
    {
        return (*this)[id].second;
    }

    /**
       Same as value(), named and behaving like std::map::at(): returns
       the value (second) of the element with the specified id.  Never
       invalidates references or iterators.

       @param id id of the element

       @return const reference to the element's second member

       @exception std::out_of_range the id is not in the SparseVectorMap
     */
    template <typename U = classT, EnableIfPair<U> = 0>
    const PairValueT<U>& at(const keyT& id) const
    {
        return (*this)[id].second;
    }

    /**
       Looks up the value (second) of the element with the specified
       id.  Never modifies the map, so unlike find() it is safe to call
       concurrently with other const access.  Never invalidates
       references or iterators.

       @param id id of the element

       @return pointer to the element's second member, or nullptr if
       the id is not in the SparseVectorMap
     */
    template <typename U = classT, EnableIfPair<U> = 0>
    PairValueT<U>* findValue(const keyT& id)
    {
        size_t index = findIndex(id);
        return index == npos ? nullptr : &data[index].second;
    }

    /**
       Looks up the value (second) of the element with the specified
       id.  Never modifies the map, so unlike find() it is safe to call
       concurrently with other const access.  Never invalidates
       references or iterators.

       @param id id of the element

       @return pointer to the element's second member, or nullptr if
       the id is not in the SparseVectorMap
     */
    template <typename U = classT, EnableIfPair<U> = 0>
    const PairValueT<U>* findValue(const keyT& id) const
    {
        size_t index = findIndex(id);
        return index == npos ? nullptr : &data[index].second;
    }

    /**
       Inserts an element with the specified id and value, or, if the
       id is already present, replaces its value.  May invalidate all
       references and iterators into the map when it inserts; replacing
       an existing value invalidates nothing.

       @param id id of the element

       @param val value to store as the element's second member

       @return reference to the element's second member
     */
    template <typename U = classT, EnableIfPair<U> = 0>
    PairValueT<U>& insert_or_assign(const keyT& id, const PairValueT<U>& val)
    {
        size_t index = findIndex(id);
        if ( index != npos ) {
            data[index].second = val;
            return data[index].second;
        }
        return insert(classT(id, val)).second;
    }

    /**
       Inserts an element with the specified id and value, like
       insert(classT(id, val)): if the id is already present, the
       existing value is left unchanged.  May invalidate all references
       and iterators into the map.

       @param id id of the element

       @param val value to store as the element's second member

       @return reference to the second member of the inserted element,
       or of the existing element if the id was already present
     */
    template <typename U = classT, EnableIfPair<U> = 0>
    PairValueT<U>& insert(const keyT& id, const PairValueT<U>& val)
    {
        return insert(classT(id, val)).second;
    }

    /**
       Finds the element with the specified id.  Merges any pending
       tail first, which MAY INVALIDATE all references and iterators
       previously obtained from the map.

       @param id id to search for

       @return iterator to the element if found, or end() if not found.
     */
    iterator find(const keyT& id)
    {
        mergeTail();
        size_t index = findIn(id, 0, data.size());
        if ( index == npos ) return data.end();
        return data.begin() + index;
    }

    /**
       Finds the element with the specified id.  Merges any pending
       tail first, which MAY INVALIDATE all references and iterators
       previously obtained from the map.

       @param id id to search for

       @return const_iterator to the element if found, or end() if not found.
     */
    const_iterator find(const keyT& id) const
    {
        mergeTail();
        size_t index = findIn(id, 0, data.size());
        if ( index == npos ) return data.end();
        return data.begin() + index;
    }

    /**
       Clears the contents of the SparseVectorMap
     */
    void clear()
    {
        data.clear();
        boundary_  = 0;
        tailLimit_ = kMinTailSize;
    }

    /**
       Removes the element with the specified id, if present.  Does
       not delete the pointee for pointer-stored elements --
       SparseVectorMap never takes ownership of pointers.  May
       invalidate all references and iterators into the map.

       @param id id of the element to remove

       @return number of elements removed (0 or 1, since keys are unique).
     */
    size_t erase(const keyT& id)
    {
        size_t pos = findIndex(id);
        if ( pos == npos ) return 0;
        data.erase(data.begin() + pos);
        if ( pos < boundary_ ) --boundary_;
        return 1;
    }

    /**
       Returns the number of items in the SparseVectorMap

       @return number of items
     */
    size_t size() const { return data.size(); }

    /**
       Checks whether the SparseVectorMap contains no elements.

       @return true if the SparseVectorMap is empty.
     */
    bool empty() const { return data.empty(); }

    /**
       Function to filter the contents of the SparseVectorMap.  Only
       available when classT is a pointer type.  Takes an object with
       an overloaded operator() function that takes as argument the
       current item.  The function returns the object that should
       take the place of this object (value returned by key()
       function must be same for both objects), or returns nullptr if
       the object should be deleted.  When an item is deleted, the
       size of the map reduces by 1.  Invalidates all references and
       iterators.

       @param filt Filter object to use for filtering contents of map

       @exception bad_filtered_key_error filt returned an object with
       a different key() than the one it was given. This is
       unrecoverable and leaves the SparseVectorMap with lost data --
       see bad_filtered_key_error's own documentation. filt must be
       implemented correctly; this is not a condition the caller can
       usefully catch and continue past.
    */
    template <typename filterT, typename U = classT, typename std::enable_if<std::is_pointer<U>::value, int>::type = 0>
    void filter(filterT& filt)
    {
        mergeTail();
        size_t write_ptr = 0;
        for ( size_t i = 0; i < data.size(); ++i ) {
            auto key        = getKey(data[i]);
            data[write_ptr] = filt(data[i]);
            if ( data[write_ptr] != nullptr ) {
                if ( UNLIKELY(getKey(data[write_ptr]) != key) ) {
                    // No recovering from this: the in-place compaction
                    // above has already overwritten data[write_ptr],
                    // and data.resize()/boundary_ never run below, so
                    // this leaves the map with lost data in an
                    // unspecified state -- see bad_filtered_key_error.
                    throw bad_filtered_key_error(
                        "ERROR: Filter object passed to SparseVectorMap::filter returned an object that had a "
                        "different value of key() than what was passed into the filter.  The filter object must return "
                        "either an object with the same value of key(), or nullptr if the object should be removed.");
                }
                write_ptr++;
            }
        }
        data.resize(write_ptr);
        data.shrink_to_fit();
        boundary_ = data.size();
    }
};

} // namespace SST

namespace SST::Core::Serialization {

template <typename keyT, typename classT, typename KeyExtract>
class serialize_impl<SST::SparseVectorMap<keyT, classT, KeyExtract>>
{
public:
    void operator()(
        SST::SparseVectorMap<keyT, classT, KeyExtract>& v, SST::Core::Serialization::serializer& ser, ser_opt_t options)
    {
        SST_SER(v.data, options);
        SST_SER(v.boundary_);
        SST_SER(v.tailLimit_);
    }
};
} // namespace SST::Core::Serialization

#endif // SST_CORE_SPARSEVECTORMAP_H
