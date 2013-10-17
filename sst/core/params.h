// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
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

class Params {
public:
    typedef std::map<std::string, std::string>::key_type key_type;
    typedef std::map<std::string, std::string>::mapped_type mapped_type;
    typedef std::map<std::string, std::string>::value_type value_type;
    typedef std::map<std::string, std::string>::key_compare key_compare;
    typedef std::map<std::string, std::string>::value_compare value_compare;
    typedef std::map<std::string, std::string>::pointer pointer;
    typedef std::map<std::string, std::string>::reference reference;
    typedef std::map<std::string, std::string>::const_reference const_reference;
    typedef std::map<std::string, std::string>::size_type size_type;
    typedef std::map<std::string, std::string>::difference_type difference_type;
    typedef std::map<std::string, std::string>::iterator iterator;
    typedef std::map<std::string, std::string>::const_iterator const_iterator;
    typedef std::map<std::string, std::string>::reverse_iterator reverse_iterator;
    typedef std::map<std::string, std::string>::const_reverse_iterator const_reverse_iterator;
    typedef std::set<key_type> KeySet_t;

    // pretend like we're a map
    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }
    reverse_iterator rbegin() { return data.rbegin(); }
    reverse_iterator rend() { return data.rend(); }
    const_reverse_iterator rbegin() const { return data.rbegin(); }
    const_reverse_iterator rend() const { return data.rend(); }
    size_type size() const { return data.size(); }
    size_type max_size() const { return data.max_size(); }
    bool empty() const { return data.empty(); }
    key_compare key_comp() const { return data.key_comp(); }
    value_compare value_comp() const { return data.value_comp(); }
    Params() : data() { }
    Params(const key_compare& comp) : data(comp) { }
    template <class InputIterator>
    Params(InputIterator f, InputIterator l) : data(f, l) { }
    template <class InputIterator>
    Params(InputIterator f, InputIterator l, const key_compare& comp) : data(f, l, comp) { }
    Params(const Params& old) : data(old.data) { }
    virtual ~Params() { }
    Params& operator=(const Params& old) { data = old.data; return *this; }
    void swap(Params& old) { data.swap(old.data); }
    std::pair<iterator, bool> insert(const value_type& x) { return data.insert(x); }
    iterator insert(iterator pos, const value_type& x) { return data.insert(pos, x); }
    template <class InputIterator>
    void insert(InputIterator f, InputIterator l) { data.insert(f, l); }
    void erase(iterator pos) {  data.erase(pos); }
    size_type erase(const key_type& k) { return data.erase(k); }
    void clear() { data.clear(); }
    iterator find(const key_type& k) { verifyParam(k); return data.find(k); }
    const_iterator find(const key_type& k) const { verifyParam(k); return data.find(k); }
    size_type count(const key_type& k) { return data.count(k); }
    iterator lower_bound(const key_type& k) { return data.lower_bound(k); }
    const_iterator lower_bound(const key_type& k) const { return data.lower_bound(k); }
    iterator upper_bound(const key_type& k) { return data.upper_bound(k); }
    const_iterator upper_bound(const key_type& k) const { return data.upper_bound(k); }
    std::pair<iterator, iterator> 
    equal_range(const key_type& k) { return data.equal_range(k); }
    std::pair<const_iterator, const_iterator> 
    equal_range(const key_type& k) const { return data.equal_range(k); }
    mapped_type& operator[](const key_type& k) { verifyParam(k); return data[k]; }
    friend bool operator==(const Params& a, const Params& b);
    friend bool operator<(const Params& a, const Params& b);

    // and have some friendly accessor functions
    long find_integer(const key_type &k, long default_value, bool &found) const {
        verifyParam(k);
        const_iterator i = data.find(k);
        if (i == data.end()) {
            found = false;
            return default_value;
        } else {
            found = true;
            return strtol(i->second.c_str(), NULL, 0);
        }
    }

    long find_integer(const key_type &k, long default_value = -1) const {
        bool tmp;
        return find_integer(k, default_value, tmp);
    }

    double find_floating(const key_type& k, double default_value, bool &found) const {
        verifyParam(k);
        const_iterator i = data.find(k);
        if (i == data.end()) {
            found = false;
            return default_value;
        } else {
            found = true;
            return strtod(i->second.c_str(), NULL);
        }
    }

    double find_floating(const key_type& k, double default_value = -1.0) const {
        bool tmp;
        return find_floating(k, default_value, tmp);
    }

    std::string find_string(const key_type &k, std::string default_value, bool &found) const {
        verifyParam(k);
        const_iterator i = data.find(k);
        if (i == data.end()) {
            found = false;
            return default_value;
        } else {
            found = true;
            return i->second;
        }
    }

    std::string find_string(const key_type &k, std::string default_value = "") const {
        bool tmp;
        return find_string(k, default_value, tmp);
    }

    void print_all_params(std::ostream &os) const {
        for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
            os << "key=" << i->first << ", value=" << i->second << std::endl;
        }
    }

    Params find_prefix_params(std::string prefix) const {
        Params ret;
        for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
            std::string key = i->first.substr(0, prefix.length());
            if (key == prefix) {
                ret[i->first.substr(prefix.length())] = i->second;
            }
        }
        ret.allowedKeys = allowedKeys;
        return ret;
    }

    // Note: This makes a COPY of the data.  Be very, very careful what you wish for!
    std::map<std::string, std::string> get_map() const { return data; }


    /***
     * @param k   Key to search for
     * @return    True if the params contains the key, false otherwise
     */
    bool contains(const key_type &k) {
        return data.find(k) != data.end();
    }

    /***
     * @param keys   Set of keys to consider valid to add to the stack
     *               of legal keys
     */
    void pushAllowedKeys(const KeySet_t &keys) {
        allowedKeys.push_back(keys);
    }

    /***
     * Removes the most recent set of keys considered allowed
     */
    void popAllowedKeys() {
        allowedKeys.pop_back();
    }

    /***
     * @param k   Key to check for validity
     * @return    True if the key is considered allowed
     */
    void verifyParam(const key_type &k) const {
        // If there are no explicitly allowed keys in the top set, allow everything
        if ( allowedKeys.empty() || allowedKeys.back().empty() ) return;

        for ( std::vector<KeySet_t>::const_reverse_iterator ri = allowedKeys.rbegin() ; ri != allowedKeys.rend() ; ++ri ) {
            if ( ri->find(k) != ri->end() ) return;
        }
        std::cerr << "Warning:  Parameter \"" << k << "\" is undocumented." << std::endl;
    }


private:
    std::map<std::string, std::string> data;
    std::vector<KeySet_t> allowedKeys;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(data);
    }

};

inline bool operator==(const Params& a, const Params& b) { return a.data == b.data; }
inline bool operator<(const Params& a, const Params& b) { return a.data < b.data; }

} //namespace SST

#endif //SST_CORE_PARAMS_H
