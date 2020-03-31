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

#ifndef SST_CORE_PARAM_H
#define SST_CORE_PARAM_H

#include "sst/core/output.h"
#include "sst/core/from_string.h"

#include <cassert>
#include <inttypes.h>
#include <iostream>
#include <sstream>
#include <map>
#include <stack>
#include <stdlib.h>
#include <utility>
#include "sst/core/threadsafe.h"

#include "sst/core/serialization/serializable.h"
#include "sst/core/serialization/serializer.h"
#include "sst/core/output.h"

int main(int argc, char *argv[]);

namespace SST {

class ConfigGraph;

/**
 * Parameter store.
 *
 * Stores key-value pairs as std::strings and provides
 * a templated find method for finding values and converting
 * them to arbitrary types (@see find()).
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


    /** Find a Parameter value in the set, and return its value as a type T.
     * Type T must be either a basic numeric type (including bool) ,
     * a std::string, or a class that has a constructor with a std::string
     * as its only parameter.  This class uses SST::Core::from_string to
     * do the conversion.
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     * @throw std::invalid_argument If value in (key, value) can't be
     * converted to type T, an invalid_argument exception is thrown.
     */
    template <class T>
    inline T find_impl(const std::string& k, T default_value, bool &found) const {
        verifyParam(k);
        // const_iterator i = data.find(getKey(k));
        const std::string& value = getString(k,found);
        // if (i == data.end()) {
            // found = false;
        if ( !found ) {
            return default_value;
        }
        else {
            // found = true;
            try {
                // return SST::Core::from_string<T>(i->second);
                return SST::Core::from_string<T>(value);
            }
            catch ( const std::invalid_argument& e ) {
                std::string msg = "Params::find(): No conversion for value: key = " + k + ", value =  " + value +
                    ".  Original error: " + e.what();
                std::invalid_argument t(msg);
                throw t;
            }
        }
    }

    /** Find a Parameter value in the set, and return its value as a type T.
     * Type T must be either a basic numeric type (including bool) ,
     * a std::string, or a class that has a constructor with a std::string
     * as its only parameter.  This class uses SST::Core::from_string to
     * do the conversion.
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found,
     *   specified as a string
     * @param found - set to true if the the parameter was found
     */
    template <class T>
    inline T find_impl(const std::string& k, const std::string& default_value, bool &found) const {
        verifyParam(k);
        const std::string& value = getString(k,found);
        if ( !found ) {
            try {
                return SST::Core::from_string<T>(default_value);
            }
            catch ( const std::invalid_argument& e ) {
                std::string msg = "Params::find(): Invalid default value specified: key = " + k + ", value =  " + default_value +
                    ".  Original error: " + e.what();
                std::invalid_argument t(msg);
                throw t;
            }
        }
        else {
            found = true;
            try {
                return SST::Core::from_string<T>(value);
            }
            catch ( const std::invalid_argument& e ) {
                std::string msg = "Params::find(): No conversion for value: key = " + k + ", value =  " + value +
                    ".  Original error: " + e.what();
                std::invalid_argument t(msg);
                throw t;
            }
        }
    }

    typedef std::map<uint32_t, std::string>::const_iterator const_iterator; /*!< Const Iterator type */

    const std::string& getString(const std::string& name, bool& found) const;

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
    size_t size() const;
    /** Returns true if the Params is empty.  (Thus begin() would equal end().) */
    bool empty() const;


    /** Create a new, empty Params */
    Params();

    /** Create a copy of a Params object */
    Params(const Params& old);

    virtual ~Params() { }

    /**
     *  @brief  Assignment operator.
     *  @param  old  Param to be copied
     *
     *  All the elements of old are copied,
     */
    Params& operator=(const Params& old);

    /**
     *  Erases all elements.
     */
    void clear();


    /**
     *  @brief  Finds the number of elements with given key.
     *  @param  k  Key of (key, value) pairs to be located.
     *  @return  Number of elements with specified key
     *  (either 1 or 0).
     *
     */
    size_t count(const key_type& k);

    /** Find a Parameter value in the set, and return its value as a type T.
     * Type T must be either a basic numeric type (including bool) ,
     * a std::string, or a class that has a constructor with a std::string
     * as its only parameter.  This class uses SST::Core::from_string to
     * do the conversion.
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     * @param found - set to true if the the parameter was found
     * @throw std::invalid_argument If value in (key, value) can't be
     * converted to type T, an invalid_argument exception is thrown.
     */
    template <class T>
    typename std::enable_if<not std::is_same<std::string,T>::value, T>::type
    find(const std::string& k, T default_value, bool &found) const {
        return find_impl<T>(k,default_value,found);
    }

    /** Find a Parameter value in the set, and return its value as a type T.
     * Type T must be either a basic numeric type (including bool) ,
     * a std::string, or a class that has a constructor with a std::string
     * as its only parameter.  This class uses SST::Core::from_string to
     * do the conversion.
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found,
     *   specified as a string
     * @param found - set to true if the the parameter was found
     */
    template <class T>
    T find(const std::string& k, const std::string& default_value, bool &found) const {
        return find_impl<T>(k,default_value,found);
    }

    /** Find a Parameter value in the set, and return its value as a type T.
     * This version of find is only enabled for bool.  This
     * is required because a string literal will be preferentially
     * cast to a bool rather than a string.  This ensures that
     * find<bool> works correctly for string literals.  This class uses
     * SST::Core::from_string to do the conversion.
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found,
     *   specified as a string literal
     */
    template <class T>
    typename std::enable_if<std::is_same<bool,T>::value, T>::type
    find(const std::string& k, const char* default_value, bool &found ) const {
        if ( nullptr == default_value ) {
            return find_impl<T>(k, static_cast<T>(0), found);
        }
        return find_impl<T>(k, std::string(default_value), found);
    }

    /** Find a Parameter value in the set, and return its value as a type T.
     * Type T must be either a basic numeric type (including bool),
     * a std::string, or a class that has a constructor with a std::string
     * as its only parameter.  This class uses SST::Core::from_string to
     * do the conversion.
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found
     */
    template <class T>
    T find(const std::string& k, T default_value ) const {
        bool tmp;
        return find_impl<T>(k, default_value, tmp);
    }

    /** Find a Parameter value in the set, and return its value as a type T.
     * Type T must be either a basic numeric type (including bool) ,
     * a std::string, or a class that has a constructor with a std::string
     * as its only parameter.  This class uses SST::Core::from_string to
     * do the conversion.
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found,
     *   specified as a string
     */
    template <class T>
    T find(const std::string& k, const std::string& default_value ) const {
        bool tmp;
        return find_impl<T>(k, default_value, tmp);
    }

    /** Find a Parameter value in the set, and return its value as a type T.
     * This version of find is only enabled for bool.  This
     * is required because a string literal will be preferentially
     * cast to a bool rather than a string.  This ensures that
     * find<bool> works correctly for string literals.This class uses
     * SST::Core::from_string to do the conversion.
     * @param k - Parameter name
     * @param default_value - Default value to return if parameter isn't found,
     *   specified as a string literal
     */
    template <class T>
    typename std::enable_if<std::is_same<bool,T>::value, T>::type
    find(const std::string& k, const char* default_value ) const {
        bool tmp;
        if ( nullptr == default_value ) {
            return find_impl<T>(k, static_cast<T>(0), tmp);
        }
        return find_impl<T>(k, std::string(default_value), tmp);
    }

    /** Find a Parameter value in the set, and return its value as a type T.
     * Type T must be either a basic numeric type (including bool) ,
     * a std::string, or a class that has a constructor with a std::string
     * as its only parameter.  This class uses SST::Core::from_string to
     * do the conversion.
     * @param k - Parameter name
     */
    template <class T>
    T find(const std::string& k) const {
        bool tmp;
        T default_value = T();
        return find_impl<T>(k, default_value, tmp);
    }

    /** Find a Parameter value in the set, and return its value as a
     * type T.  Type T must be either a basic numeric type , a
     * std::string, or a class that has a constructor with a
     * std::string as its only parameter.  This version of find is not
     * enabled for bool as it conflicts with find<bool>(string key, bool
     * default_value).  This class uses SST::Core::from_string to do
     * the conversion.
     * @param k - Parameter name
     * @param found - set to true if the the parameter was found
     */
    template <class T>
    typename std::enable_if<not std::is_same<bool, T>::value, T>::type
    find(const std::string& k, bool &found) const {
        T default_value = T();
        return find_impl<T>(k, default_value, found);
    }

    /** Find a Parameter value in the set, and return its value as a
     * vector of T's.  The array will be appended to
     * the end of the vector.
     * Type T must be either a basic numeric type (including bool) ,
     * a std::string, or a class that has a constructor with a std::string
     * as its only parameter.  This class uses SST::Core::from_string to
     * do the conversion.
     *
     * @param k - Parameter name
     * @param vec - vector to append array items to
     */
    template <class T>
    void find_array(const key_type &k, std::vector<T>& vec) const {
        verifyParam(k);
        // const_iterator i = data.find(getKey(k));
        // if ( i == data.end()) {
        //     return;
        // }
        // std::string value = i->second;
        bool found = false;
        std::string value = getString(k,found);
        if ( !found ) return;
        // String should start with [ and end with ], we need to cut
        // these out
        // Test the value for correct [...] formatting
        if( (value.find("[") == std::string::npos) ||
            (value.find("]") == std::string::npos) ){
          std::string msg =
            "Params::find_array(): Invalid formatting: String must be enclosed by brackets [str]";
          std::invalid_argument t(msg);
          throw t;
        }
        value = value.substr(0,value.size()-1);
        value = value.substr(1);

        std::stringstream ss(value);

        while( ss.good() ) {
            std::string substr;
            getline( ss, substr, ',' );
            // vec.push_back(strtol(substr.c_str(), nullptr, 0));
            try {
                vec.push_back(SST::Core::from_string<T>(substr));
            }
            catch ( const std::invalid_argument& e ) {
                std::string msg = "Params::find(): No conversion for value: key = " + k + ", value =  " + substr +
                    ".  Original error: " + e.what();
                std::invalid_argument t(msg);
                throw t;
            }
        }
    }

    /** Print all key/value parameter pairs to specified ostream */
    void print_all_params(std::ostream &os, const std::string& prefix = "") const;
    void print_all_params(Output &out, const std::string& prefix = "") const;



    /** Add a key value pair into the param object.
     */
    void insert(const std::string& key, const std::string& value, bool overwrite = true);

    void insert(const Params& params);

    std::set<std::string> getKeys() const;

     /** Returns a new parameter object with parameters that match
     * the specified prefix.
     */
    Params find_prefix_params(const std::string& prefix) const;

    Params find_scoped_params(const std::string& scope, const char* delims = ".:") const;

    /**
     * @param k   Key to search for
     * @return    True if the params contains the key, false otherwise
     */
    bool contains(const key_type &k);

    /**
     * @param keys   Set of keys to consider valid to add to the stack
     *               of legal keys
     */
    void pushAllowedKeys(const KeySet_t &keys);

    /**
     * Removes the most recent set of keys considered allowed
     */
    void popAllowedKeys();

    /**
     * @param k   Key to check for validity
     * @return    True if the key is considered allowed
     */
    void verifyParam(const key_type &k) const;


    /**
     * Given a Parameter Key ID, return the Name of the matching parameter
     * @param id  Key ID to look up
     * @return    String name of the parameter
     */
    static const std::string& getParamName(uint32_t id);


    void serialize_order(SST::Core::Serialization::serializer &ser) override;
    ImplementSerializable(SST::Params)

private:
    std::map<uint32_t, std::string> data;
    std::vector<KeySet_t> allowedKeys;
    bool verify_enabled;
    static bool g_verify_enabled;

    uint32_t getKey(const std::string& str) const;
    uint32_t getKey(const std::string& str);

    /* Friend main() because it broadcasts the maps */
    friend int ::main(int argc, char *argv[]);

    static std::map<std::string, uint32_t> keyMap;
    static std::vector<std::string> keyMapReverse;
    static SST::Core::ThreadSafe::Spinlock keyLock;
    static uint32_t nextKeyID;


};

#if 0
 class UnitAlgebra;

 #define SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(type) \
     template<> \
     type Params::find(const std::string& k, type default_value, bool &found) const; \
     template<> \
     type Params::find(const std::string& k, const std::string& default_value, bool &found) const; \
     template <> \
     type Params::find(const std::string& k, type default_value ) const; \
     template <> \
     type Params::find(const std::string& k, const std::string& default_value ) const; \
     template <> \
     type Params::find(const std::string& k) const;


 SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(int32_t)
 SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(uint32_t)
 SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(int64_t)
 SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(uint64_t)
 SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(bool)
 SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(float)
 SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(double)
 SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(UnitAlgebra)

 // std::string has to be special cased because of signature conflicts
 // SST_PARAMS_DECLARE_TEMPLATE_SPECIALIZATION(std::string)
 template<>
 std::string Params::find<std::string>(const std::string& k, const std::string& default_value, bool &found) const;
#endif



} //namespace SST

#endif //SST_CORE_PARAMS_H
