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

#ifndef SST_PARAM_H
#define SST_PARAM_H

#include <map>
#include <set>
#include <iostream>
#include <utility>

#include <stdlib.h>


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
    iterator find(const key_type& k) { return data.find(k); }
    const_iterator find(const key_type& k) const { return data.find(k); }
    size_type count(const key_type& k) { return data.count(k); }
    iterator lower_bound(const key_type& k) { return data.lower_bound(k); }
    const_iterator lower_bound(const key_type& k) const { return data.lower_bound(k); }
    iterator upper_bound(const key_type& k) { return data.upper_bound(k); }
    const_iterator upper_bound(const key_type& k) const { return data.upper_bound(k); }
    std::pair<iterator, iterator> 
    equal_range(const key_type& k) { return data.equal_range(k); }
    std::pair<const_iterator, const_iterator> 
    equal_range(const key_type& k) const { return data.equal_range(k); }
    mapped_type& operator[](const key_type& k) { return data[k]; }
    friend bool operator==(const Params& a, const Params& b);
    friend bool operator<(const Params& a, const Params& b);

    // and have some friendly accessor functions
    long find_integer(const key_type &k, long default_value, bool &found) const {
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
        return ret;
    }

    // Note: This makes a COPY of the data.  Be very, very careful what you wish for!
    std::map<std::string, std::string> get_map() const { return data; }

    void verify_params(std::set<std::string> info_params, std::string obj_name) {
        const_iterator i, end = data.end();

        for (i = data.begin() ; i != end ; ++i) {
            if (info_params.end() == info_params.find(i->first)) {
                std::cerr << "Warning: Invalid Parameter \"" << i->first
                          << "\" specified for " << obj_name << "." << std::endl;
            }
        }
    }

private:
    std::map<std::string, std::string> data;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(data);
    }
};

inline bool operator==(const Params& a, const Params& b) { return a.data == b.data; }
inline bool operator<(const Params& a, const Params& b) { return a.data < b.data; }

}

#endif
