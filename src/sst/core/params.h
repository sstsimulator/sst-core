// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_PARAM_H
#define SST_CORE_PARAM_H

#include <sst/core/serialization.h>

#include <sst/core/output.h>
#include <sst/core/from_string.h>

#include <inttypes.h>
#include <iostream>
#include <sstream>
#include <map>
#include <stack>
#include <stdlib.h>
#include <utility>
#include <sst/core/threadsafe.h>

#include <sst/core/serialization/serializable.h>
#include <sst/core/serialization/serializer.h>

int main(int argc, char *argv[]);

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
class Params : public SST::Core::Serialization::serializable {
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

    typedef std::map<uint32_t, std::string>::const_iterator const_iterator; /*!< Const Iterator type */

public:
    typedef std::string key_type;  /*!< Type of key (string) */
    typedef std::set<key_type, KeyCompare> KeySet_t; /*!< Type of a set of keys */

    /**
     * Enable or disable parameter verification on an instance
     * of Params.  Useful when generating a new set of Params to
     * pass off to a module.
     */
    bool enableVerify(bool enable) { bool old = verify_enabled; verify_enabled = enable; return old; }

    /**
     * Enable, on a global scale, parameter verification.  Used
     * after construction of the config graph so that warnings are
     * not generated during construction.
     */
    static void enableVerify() { g_verify_enabled = true; };

    /** Returns the size of the Params.  */
    size_t size() const { return data.size(); }
    /** Returns true if the Params is empty.  (Thus begin() would equal end().) */
    bool empty() const { return data.empty(); }


    /** Create a new, empty Params */
    Params() : data(), verify_enabled(true) { }

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
     *  Erases all elements in a %map.  Note that this function only
     *  erases the elements, and that if the elements themselves are
     *  pointers, the pointed-to memory is not touched in any way.
     *  Managing the pointer is the user's responsibilty.
     */
    void clear() { data.clear(); }


    /**
     *  @brief  Finds the number of elements with given key.
     *  @param  k  Key of (key, value) pairs to be located.
     *  @return  Number of elements with specified key.
     *
     *  This function only makes sense for multimaps; for map the result will
     *  either be 0 (not present) or 1 (present).
     */
    size_t count(const key_type& k) { return data.count(getKey(k)); }

    /** Find a Parameter value in the set, and return its value as a type T
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     */
    template <class T>
    T find(const std::string &k, T default_value, bool &found) const {
        verifyParam(k);
        const_iterator i = data.find(getKey(k));
        if (i == data.end()) {
            found = false;
            return default_value;
        } else {
            found = true;
            try {
                return SST::Core::from_string<T>(i->second);
            }
            catch ( const std::invalid_argument& e ) {
                std::string msg = "Params::find(): No conversion for value: key = " + k + ", value =  " + i->second +
                    ".  Oringal error: " + e.what();
                std::invalid_argument t(msg);
                throw t;
            }
        }        
    }
    
    /** Find a Parameter value in the set, and return its value as a type T
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     */
    template <class T>
    T find(const std::string &k, T default_value ) const {
        bool tmp;
        return find(k, default_value, tmp);
    }
    
    /** Find a Parameter value in the set, and return its value as a type T
     * @param k - Parameter name
     */
    template <class T>
    T find(const std::string &k) const {
        bool tmp;
        T default_value = T();
        return find(k, default_value, tmp);
    }
    
    /** Find a Parameter value in the set, and return its value as a type T
     * @param k - Parameter name
     * @param found - set to true if the the parameter was found
     */
    template <class T>
    T find(const std::string &k, bool &found) const {
        T default_value = T();
        return find(k, default_value, found);
    }

    /** Find a Parameter value in the set, and return its value as a
     * vector of T's.  The array will be appended to
     * the end of the vector.
     *
     * @param k - Parameter name
     * @param vec - vector to append array items to
     */
    template <class T>
    void find_array(const key_type &k, std::vector<T>& vec) const {
        verifyParam(k);
        const_iterator i = data.find(getKey(k));
        if ( i == data.end()) {
            return;
        }
        std::string value = i->second;
        // String should start with [ and end with ], we need to cut
        // these out
        value = value.substr(0,value.size()-1);
        value = value.substr(1);
        
        std::stringstream ss(value);
        
        while( ss.good() ) {
            std::string substr;
            getline( ss, substr, ',' );
            // vec.push_back(strtol(substr.c_str(), NULL, 0));
            try {
                vec.push_back(SST::Core::from_string<T>(substr));
            }
            catch ( const std::invalid_argument& e ) {
                std::string msg = "Params::find(): No conversion for value: key = " + k + ", value =  " + substr +
                    ".  Oringal error: " + e.what();
                std::invalid_argument t(msg);
                throw t;
            }
        }
    }

    /** Find a Parameter value in the set, and return its value as an integer
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     */
    __attribute__ ((deprecated))
    int64_t find_integer(const key_type &k, long default_value, bool &found) const {
        return find<int64_t>(k,default_value,found);
    }

    /** Find a Parameter value in the set, and return its value as an integer
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     */
    __attribute__ ((deprecated))
    int64_t find_integer(const key_type &k, long default_value = -1) const {
        return find<int64_t>(k,default_value);
    }

    /** Find a Parameter value in the set, and return its value as a
     * vector of integers.  The array of integers will be appended to
     * the end of the vector.
     *
     * @param k - Parameter name
     * @param vec - vector to append array items to
     */
    __attribute__ ((deprecated))
    void find_integer_array(const key_type &k, std::vector<int64_t>& vec) const {
        find_array<int64_t>(k,vec);
    }

    /** Find a Parameter value in the set, and return its value as a double
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     */
    __attribute__ ((deprecated))
    double find_floating(const key_type& k, double default_value, bool &found) const {
        return find<double>(k,default_value,found);
    }

    /** Find a Parameter value in the set, and return its value as a double
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     */
    __attribute__ ((deprecated))
    double find_floating(const key_type& k, double default_value = -1.0) const {
        return find<double>(k,default_value);
    }

    /** Find a Parameter value in the set, and return its value as a
     * vector of floats.  The array of floats will be appended to
     * the end of the vector.
     *
     * @param k - Parameter name
     * @param vec - vector to append array items to
     */
    __attribute__ ((deprecated))
    void find_floating_array(const key_type &k, std::vector<double>& vec) const {
        find_array<double>(k,vec);
    }

    /** Find a Parameter value in the set, and return its value
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     */
    __attribute__ ((deprecated))
    std::string find_string(const key_type &k, std::string default_value, bool &found) const {
        return find<std::string>(k,default_value,found);
    }

    /** Find a Parameter value in the set, and return its value
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     */
    __attribute__ ((deprecated))
    std::string find_string(const key_type &k, std::string default_value = "") const {
        return find<std::string>(k,default_value);
    }

    /** Find a Parameter value in the set, and return its value as a
     * vector of strings.  The array of strings will be appended to
     * the end of the vector.
     *
     * @param k - Parameter name
     * @param vec - vector to append array items to
     */
    __attribute__ ((deprecated))
    void find_string_array(const key_type &k, std::vector<std::string>& vec) const {
        find_array<std::string>(k,vec);
    }

    /** Print all key/value parameter pairs to specified ostream */
    void print_all_params(std::ostream &os, std::string prefix = "") const {
        for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
            os << prefix << "key=" << keyMapReverse[i->first] << ", value=" << i->second << std::endl;
        }
    }



    /** Add a key value pair into the param object.
     */
    void insert(std::string key, std::string value, bool overwrite = true) {
        if ( overwrite ) {
            data[getKey(key)] = value;
        }
        else {
            uint32_t id = getKey(key);
            data.insert(std::make_pair(id, value));
        }
    }

    void insert(const Params& params) {
        data.insert(params.data.begin(), params.data.end());
    }

    std::set<std::string> getKeys() const {
        std::set<std::string> ret;
        for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
            ret.insert(keyMapReverse[i->first]);
        }
        return ret;
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
                ret.insert(keyMapReverse[i->first].substr(prefix.length()), i->second);
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

#ifdef USE_PARAM_WARNINGS
        SST::Output outXX("ParamWarning: ", 0, 0, Output::STDERR);
        outXX.output(CALL_INFO, "Warning: Parameter \"%s\" is undocumented.\n", k.c_str());
#endif
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


    void serialize_order(SST::Core::Serialization::serializer &ser) {
        std::cout << "Params: starting serialization of data" << std::endl;
        ser & data;
        std::cout << "Params: done with serialization of data" << std::endl;
    }    
    
    ImplementSerializable(SST::Params)

private:
    std::map<uint32_t, std::string> data;
    std::vector<KeySet_t> allowedKeys;
    bool verify_enabled;
    static bool g_verify_enabled;

    uint32_t getKey(const std::string &str) const
    {
        std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(keyLock);
        std::map<std::string, uint32_t>::iterator i = keyMap.find(str);
        if ( i == keyMap.end() ) {
            return (uint32_t)-1;
        }
        return i->second;
    }

    uint32_t getKey(const std::string &str)
    {
        std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(keyLock);
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

    /* Friend main() because it broadcasts the maps */
    friend int ::main(int argc, char *argv[]);

    static std::map<std::string, uint32_t> keyMap;
    static std::vector<std::string> keyMapReverse;
    static SST::Core::ThreadSafe::Spinlock keyLock;
    static uint32_t nextKeyID;


};

} //namespace SST

#endif //SST_CORE_PARAMS_H
