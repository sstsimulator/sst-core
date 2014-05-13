// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_PARAM_H
#define SST_CORE_PARAM_H

#include <sst/core/serialization.h>

#include <iostream>
#include <map>
#include <stack>
#include <stdlib.h>
#include <utility>

namespace SST {

class ConfigGraph;

/**
 * Parameter store
 *
 *  Meets the requirements of a <a href="tables.html#65">container</a>, a
 *  <a href="tables.html#66">reversible container</a>, and an
 *  <a href="tables.html#69">associative container</a> (using unique keys).
 *  For a @c map<Key,T> the key_type is Key, the mapped_type is T, and the
 *  value_type is std::pair<const Key,T>.
 */
class Params {
private:
    struct KeyCompare : std::binary_function<std::string, std::string, bool>
    {
        bool operator()(const std::string& X, const std::string& Y) const
        {
            const char *x = X.c_str();
            const char *y = Y.c_str();

#define EAT_VAR(A, B) \
            do { \
                if ( *x == '%' && (*(x+1) == '(' || *(x+1) == 'd')) {   \
                    /* We need to eat off some tokens */                \
                    ++x;                                                \
                    if ( *x == '(' ) {                                  \
                        do { x++; } while ( *x && *x != ')' );          \
                        x++; /* *x should now == 'd' */                 \
                    }                                                   \
                    if ( *x != 'd' ) goto NO_VARIABLE;                  \
                    x++; /* Finish eating the variable */               \
                    /* Now, eat of digits of Y */                       \
                    while ( *y && isdigit(*y) ) y++;                    \
                }                                                       \
            } while(0)

            do {
                EAT_VAR(x, y);
                EAT_VAR(y, x);
NO_VARIABLE:
                if ( *x == *y ) {
                    if ( '\0' == *x ) return false;
                    x++;
                    y++;
                } else {
                    if ( *x < *y ) return true;
                    return false;
                }
            } while ( *x && *y );
            if ( !(*x) && (*y) ) return true;
            return false;

#undef EAT_VAR
        }
    };

public:
    typedef std::map<std::string, std::string>::key_type key_type;  /*!< Type of key (string) */
    typedef std::map<uint32_t, std::string>::mapped_type mapped_type; /*!< Type of value (string) */
    typedef std::map<std::string, std::string>::value_type value_type; /*!< Pair of strings */
    typedef std::map<uint32_t, std::string>::key_compare key_compare; /*!< Key comparator type */
    typedef std::map<uint32_t, std::string>::value_compare value_compare; /*!< Value comparator type */
    typedef std::map<uint32_t, std::string>::pointer pointer; /*!< Pointer type */
    typedef std::map<uint32_t, std::string>::reference reference; /*!< Reference type */
    typedef std::map<uint32_t, std::string>::const_reference const_reference; /*!< Const Reference type */
    typedef std::map<uint32_t, std::string>::size_type size_type; /*!< Size type */
    typedef std::map<uint32_t, std::string>::difference_type difference_type; /*!< Difference type */
    typedef std::map<uint32_t, std::string>::iterator iterator; /*!< Iterator type */
    typedef std::map<uint32_t, std::string>::const_iterator const_iterator; /*!< Const Iterator type */
    typedef std::map<uint32_t, std::string>::reverse_iterator reverse_iterator; /*!< Reverse Iterator type */
    typedef std::map<uint32_t, std::string>::const_reverse_iterator const_reverse_iterator; /*!< Const Reverse Iterator type */
    typedef std::set<key_type, KeyCompare> KeySet_t; /*!< Type of a set of keys */

    /**
     * Enable or disable parameter verification on an instance
     * of Params.  Useful when generating a new set of Params to
     * pass off to a module.
     */
    void enableVerify(bool enable) { verify_enabled = enable; };

    /**
     * Enable, on a global scale, parameter verification.  Used
     * after construction of the config graph so that warnings are
     * not generated during construction.
     */
    static void enableVerify() { g_verify_enabled = true; };

    // pretend like we're a map
    /** Returns a read/write iterator that points to the first pair in the
     *  Params.
     *  Iteration is done in ascending order according to the keys.
     */
    iterator begin() { return data.begin(); }
    /**  Returns a read/write iterator that points one past the last
     *  pair in the Params.  Iteration is done in ascending order
     *  according to the keys.
     */
    iterator end() { return data.end(); }
    /** Returns a read-only (constant) iterator that points to the first pair
     *  in the Params.  Iteration is done in ascending order according to the
     *  keys.
     */
    const_iterator begin() const { return data.begin(); }
    /** Returns a read-only (constant) iterator that points one past the last
     *  pair in the Params.  Iteration is done in ascending order according to
     *  the keys.
     */
    const_iterator end() const { return data.end(); }
    /** Returns a read/write reverse iterator that points to the last pair in
     *  the Params.  Iteration is done in descending order according to the
     *  keys.
     */
    reverse_iterator rbegin() { return data.rbegin(); }
    /** Returns a read/write reverse iterator that points to one before the
     *  first pair in the Params.  Iteration is done in descending order
     *  according to the keys.
     */
    reverse_iterator rend() { return data.rend(); }
    /** Returns a read-only (constant) reverse iterator that points to the
     *  last pair in the Params.  Iteration is done in descending order
     *  according to the keys.
     */
    const_reverse_iterator rbegin() const { return data.rbegin(); }
    /** Returns a read-only (constant) reverse iterator that points to one
     *  before the first pair in the Params.  Iteration is done in descending
     *  order according to the keys.
     */
    const_reverse_iterator rend() const { return data.rend(); }
    /** Returns the size of the Params.  */
    size_type size() const { return data.size(); }
    /** Returns the maximum size of the Params.  */
    size_type max_size() const { return data.max_size(); }
    /** Returns true if the Params is empty.  (Thus begin() would equal end().) */
    bool empty() const { return data.empty(); }


    /** Create a new, empty Params */
    Params() : data(), verify_enabled(true) { }

    /** Create a new, empty Params with specified key comparison functor */
    Params(const key_compare& comp) : data(comp), verify_enabled(true) { }


    /** Create a copy of a Params object */
    Params(const Params& old) : data(old.data), allowedKeys(old.allowedKeys), verify_enabled(old.verify_enabled) { }

    virtual ~Params() { }

    /**
     *  @brief  Map assignment operator.
     *  @param  old  A %map of identical element and allocator types.
     *
     *  All the elements of @a x are copied, but unlike the copy constructor,
     *  the allocator object is not copied.
     */
    Params& operator=(const Params& old) {
        data = old.data;
        verify_enabled = old.verify_enabled;
        allowedKeys = old.allowedKeys;
        return *this;
    }

    /**
     *  @brief Attempts to insert a std::pair into the %map.
     *
     *  @param  x  Pair to be inserted (see std::make_pair for easy creation 
     *         of pairs).
     *
     *  @return  A pair, of which the first element is an iterator that 
     *           points to the possibly inserted pair, and the second is 
     *           a bool that is true if the pair was actually inserted.
     *
     *  This function attempts to insert a (key, value) %pair into the %map.
     *  A %map relies on unique keys and thus a %pair is only inserted if its
     *  first element (the key) is not already present in the %map.
     *
     *  Insertion requires logarithmic time.
     */
    std::pair<iterator, bool> insert(const value_type& x) {
        uint32_t id = getKey(x.first);
        return data.insert(std::make_pair(id, x.second));
    }

    /**
     *  @brief Attempts to insert a std::pair into the %map.
     *  @param  pos  An iterator that serves as a hint as to where the
     *                    pair should be inserted.
     *  @param  x  Pair to be inserted (see std::make_pair for easy creation
     *             of pairs).
     *  @return  An iterator that points to the element with key of @a x (may
     *           or may not be the %pair passed in).
     *
     *
     *  This function is not concerned about whether the insertion
     *  took place, and thus does not return a boolean like the
     *  single-argument insert() does.  Note that the first
     *  parameter is only a hint and can potentially improve the
     *  performance of the insertion process.  A bad hint would
     *  cause no gains in efficiency.
     *
     *  Insertion requires logarithmic time (if the hint is not taken).
     */
    iterator insert(iterator pos, const value_type& x) {
        uint32_t id = getKey(x.first);
        return data.insert(pos, std::make_pair(id, x.second));
    }
    /**
     *  @brief Template function that attemps to insert a range of elements.
     *  @param  f  Iterator pointing to the start of the range to be
     *                 inserted.
     *  @param  l  Iterator pointing to the end of the range.
     *
     *  Complexity similar to that of the range constructor.
     */
    template <class InputIterator>
    void insert(InputIterator f, InputIterator l) {
        data.insert(f, l);
    }

    /**
     *  @brief Erases an element from a %map.
     *  @param  pos  An iterator pointing to the element to be erased.
     *
     *  This function erases an element, pointed to by the given
     *  iterator, from a %map.  Note that this function only erases
     *  the element, and that if the element is itself a pointer,
     *  the pointed-to memory is not touched in any way.  Managing
     *  the pointer is the user's responsibilty.
     */
    void erase(iterator pos) {  data.erase(pos); }
    /**
     *  @brief Erases elements according to the provided key.
     *  @param  k  Key of element to be erased.
     *  @return  The number of elements erased.
     *
     *  This function erases all the elements located by the given key from
     *  a %map.
     *  Note that this function only erases the element, and that if
     *  the element is itself a pointer, the pointed-to memory is not touched
     *  in any way.  Managing the pointer is the user's responsibilty.
     */
    size_type erase(const key_type& k) { return data.erase(getKey(k)); }
    /**
     *  Erases all elements in a %map.  Note that this function only
     *  erases the elements, and that if the elements themselves are
     *  pointers, the pointed-to memory is not touched in any way.
     *  Managing the pointer is the user's responsibilty.
     */
    void clear() { data.clear(); }


    /**
     *  @brief Tries to locate an element in a %map.
     *  @param  k  Key of (key, value) %pair to be located.
     *  @return  Iterator pointing to sought-after element, or end() if not
     *           found.
     *
     *  This function takes a key and tries to locate the element with which
     *  the key matches.  If successful the function returns an iterator
     *  pointing to the sought after %pair.  If unsuccessful it returns the
     *  past-the-end ( @c end() ) iterator.
     */
    iterator find(const key_type& k) { verifyParam(k); return data.find(getKey(k)); }
    /**
     *  @brief Tries to locate an element in a %map.
     *  @param  k  Key of (key, value) %pair to be located.
     *  @return  Read-only (constant) iterator pointing to sought-after
     *           element, or end() if not found.
     *
     *  This function takes a key and tries to locate the element with which
     *  the key matches.  If successful the function returns a constant
     *  iterator pointing to the sought after %pair. If unsuccessful it
     *  returns the past-the-end ( @c end() ) iterator.
     */
    const_iterator find(const key_type& k) const { verifyParam(k); return data.find(getKey(k)); }
    /**
     *  @brief  Finds the number of elements with given key.
     *  @param  k  Key of (key, value) pairs to be located.
     *  @return  Number of elements with specified key.
     *
     *  This function only makes sense for multimaps; for map the result will
     *  either be 0 (not present) or 1 (present).
     */
    size_type count(const key_type& k) { return data.count(getKey(k)); }
    /**
     *  @brief  Subscript ( @c [] ) access to %map data.
     *  @param  k  The key for which data should be retrieved.
     *  @return  A reference to the data of the (key,data) %pair.
     *
     *  Allows for easy lookup with the subscript ( @c [] )
     *  operator.  Returns data associated with the key specified in
     *  subscript.  If the key does not exist, a pair with that key
     *  is created using default values, which is then returned.
     *
     *  Lookup requires logarithmic time.
     */
    mapped_type& operator[](const key_type& k) { verifyParam(k); return data[getKey(k)]; }

    /** Find a Parameter value in the set, and return its value as an integer
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     */
    long find_integer(const key_type &k, long default_value, bool &found) const {
        verifyParam(k);
        const_iterator i = data.find(getKey(k));
        if (i == data.end()) {
            found = false;
            return default_value;
        } else {
            found = true;
            return strtol(i->second.c_str(), NULL, 0);
        }
    }

    /** Find a Parameter value in the set, and return its value as an integer
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     */
    long find_integer(const key_type &k, long default_value = -1) const {
        bool tmp;
        return find_integer(k, default_value, tmp);
    }

    /** Find a Parameter value in the set, and return its value as a double
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     */
    double find_floating(const key_type& k, double default_value, bool &found) const {
        verifyParam(k);
        const_iterator i = data.find(getKey(k));
        if (i == data.end()) {
            found = false;
            return default_value;
        } else {
            found = true;
            return strtod(i->second.c_str(), NULL);
        }
    }

    /** Find a Parameter value in the set, and return its value as a double
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     */
    double find_floating(const key_type& k, double default_value = -1.0) const {
        bool tmp;
        return find_floating(k, default_value, tmp);
    }

    /** Find a Parameter value in the set, and return its value
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     */
    std::string find_string(const key_type &k, std::string default_value, bool &found) const {
        verifyParam(k);
        const_iterator i = data.find(getKey(k));
        if (i == data.end()) {
            found = false;
            return default_value;
        } else {
            found = true;
            return i->second;
        }
    }

    /** Find a Parameter value in the set, and return its value
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     */
    std::string find_string(const key_type &k, std::string default_value = "") const {
        bool tmp;
        return find_string(k, default_value, tmp);
    }

    /** Print all key/value parameter pairs to specified ostream */
    void print_all_params(std::ostream &os) const {
        for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
            os << "key=" << keyMapReverse[i->first] << ", value=" << i->second << std::endl;
        }
    }

    /** Returns a new parameter object with parameters that match
     * the specified prefix.
     */
    Params find_prefix_params(std::string prefix) const {
        Params ret;
        ret.enableVerify(false);
        for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
            std::string key = keyMapReverse[i->first].substr(0, prefix.length());
            if (key == prefix) {
                ret[keyMapReverse[i->first].substr(prefix.length())] = i->second;
            }
        }
        ret.allowedKeys = allowedKeys;
        ret.enableVerify(verify_enabled);

        return ret;
    }


    /**
     * @param k   Key to search for
     * @return    True if the params contains the key, false otherwise
     */
    bool contains(const key_type &k) {
        return data.find(getKey(k)) != data.end();
    }

    /**
     * @param keys   Set of keys to consider valid to add to the stack
     *               of legal keys
     */
    void pushAllowedKeys(const KeySet_t &keys) {
        allowedKeys.push_back(keys);
    }

    /**
     * Removes the most recent set of keys considered allowed
     */
    void popAllowedKeys() {
        allowedKeys.pop_back();
    }

    /**
     * @param k   Key to check for validity
     * @return    True if the key is considered allowed
     */
    void verifyParam(const key_type &k) const {
        if ( !g_verify_enabled || !verify_enabled ) return;

        for ( std::vector<KeySet_t>::const_reverse_iterator ri = allowedKeys.rbegin() ; ri != allowedKeys.rend() ; ++ri ) {
            if ( ri->find(k) != ri->end() ) return;
        }
        std::cerr << "Warning:  Parameter \"" << k << "\" is undocumented." << std::endl;
    }


    /**
     * Given a Parameter Key ID, return the Name of the matching parameter
     * @param id  Key ID to look up
     * @return    String name of the parameter
     */
    static const std::string& getParamName(uint32_t id)
    {
        return keyMapReverse[id];
    }



private:
    std::map<uint32_t, std::string> data;
    std::vector<KeySet_t> allowedKeys;
    bool verify_enabled;
    static bool g_verify_enabled;

    uint32_t getKey(const std::string &str) const
    {
        std::map<std::string, uint32_t>::iterator i = keyMap.find(str);
        if ( i == keyMap.end() ) {
            return (uint32_t)-1;
        }
        return i->second;
    }

    uint32_t getKey(const std::string &str)
    {
        std::map<std::string, uint32_t>::iterator i = keyMap.find(str);
        if ( i == keyMap.end() ) {
            uint32_t id = nextKeyID++;
            keyMap.insert(std::make_pair(str, id));
            keyMapReverse.push_back(str);
            assert(keyMapReverse.size() == nextKeyID);
            return id;
        }
        return i->second;
    }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(data);
    }


public:
	/** NOT FOR PUBLIC USE **/
    static std::map<std::string, uint32_t> keyMap;
	/** NOT FOR PUBLIC USE **/
    static std::vector<std::string> keyMapReverse;
	/** NOT FOR PUBLIC USE **/
    static uint32_t nextKeyID;


};

} //namespace SST

#endif //SST_CORE_PARAMS_H
