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
//

#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/core/unitAlgebra.h>

#include <map>
#include <vector>
#include <string>


namespace SST {

const std::string&
Params::getString(const std::string& name, bool& found) const
{
    static std::string empty;
    const_iterator i = data.find(getKey(name));
    if ( i == data.end() ) {
        found = false;
        return empty;
    }
    found = true;
    return i->second;
}

size_t
Params::size() const
{
    return data.size();
}

bool
Params::empty() const
{
    return data.empty();
}

Params::Params() :
    data(), verify_enabled(true)
{}

Params::Params(const Params& old) :
    data(old.data),
    allowedKeys(old.allowedKeys),
    verify_enabled(old.verify_enabled)
{}


Params&
Params::operator=(const Params& old) {
    data = old.data;
    verify_enabled = old.verify_enabled;
    allowedKeys = old.allowedKeys;
    return *this;
}

void
Params::clear()
{
    data.clear();
}


size_t
Params::count(const key_type& k)
{
    return data.count(getKey(k));
}

void
Params::print_all_params(std::ostream &os, std::string prefix) const
{
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        os << prefix << "key=" << keyMapReverse[i->first] << ", value=" << i->second << std::endl;
    }
}

void
Params::print_all_params(Output &out, std::string prefix) const
{
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        out.output("%s%s = %s\n", prefix.c_str(), keyMapReverse[i->first].c_str(), i->second.c_str());
    }
}


void
Params::insert(std::string key, std::string value, bool overwrite)
{
    if ( overwrite ) {
        data[getKey(key)] = value;
    }
    else {
        uint32_t id = getKey(key);
        data.insert(std::make_pair(id, value));
    }
}

void
Params::insert(const Params& params)
{
    data.insert(params.data.begin(), params.data.end());
}

std::set<std::string>
Params::getKeys() const
{
    std::set<std::string> ret;
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        ret.insert(keyMapReverse[i->first]);
    }
    return ret;
}

Params
Params::find_scoped_params(const std::string& prefix, const char* delims) const
{
    int num_delims = ::strlen(delims);
    Params ret;
    ret.enableVerify(false);
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        auto& fullKeyName = keyMapReverse[i->first];
        std::string key = fullKeyName.substr(0, prefix.length());
        auto start = prefix.length();
        if (key == prefix) {
          char next = fullKeyName[start];
          bool delimMatches = false;
          for (int i=0; i < num_delims; ++i){
            if (next == delims[i]){
              delimMatches = true;
              break;
            }
          }
          if (delimMatches){
            ret.insert(keyMapReverse[i->first].substr(start +1), i->second);
          }
        }
    }
    ret.allowedKeys = allowedKeys;
    ret.enableVerify(verify_enabled);
    return ret;
}

Params
Params::find_prefix_params(const std::string& prefix) const
{
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

bool
Params::contains(const key_type &k)
{
    return data.find(getKey(k)) != data.end();
}

void
Params::pushAllowedKeys(const KeySet_t &keys)
{
    allowedKeys.push_back(keys);
}

void
Params::popAllowedKeys()
{
    allowedKeys.pop_back();
}

void
Params::verifyParam(const key_type &k) const
{
    if ( !g_verify_enabled || !verify_enabled ) return;
    
    for ( std::vector<KeySet_t>::const_reverse_iterator ri = allowedKeys.rbegin() ; ri != allowedKeys.rend() ; ++ri ) {
        if ( ri->find(k) != ri->end() ) return;
    }
    
#ifdef USE_PARAM_WARNINGS
    SST::Output outXX("ParamWarning: ", 0, 0, Output::STDERR);
    outXX.output(CALL_INFO, "Warning: Parameter \"%s\" is undocumented.\n", k.c_str());
#endif
}

const std::string&
Params::getParamName(uint32_t id)
{
    return keyMapReverse[id];
}


void
Params::serialize_order(SST::Core::Serialization::serializer &ser)
{
    ser & data;
}    

uint32_t
Params::getKey(const std::string &str) const
{
    std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(keyLock);
    std::map<std::string, uint32_t>::iterator i = keyMap.find(str);
    if ( i == keyMap.end() ) {
        return (uint32_t)-1;
    }
    return i->second;
}

uint32_t
Params::getKey(const std::string &str)
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


// template<>
// uint32_t Params::find(const std::string &k) const
// {
//     bool tmp;
//     uint32_t default_value = uint32_t();
//     return find(k, default_value, tmp);
// }


// #define SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(type) \
//     template<> \
//     type Params::find(const std::string &k, type default_value, bool &found) const { \
//         std::cout << "******* specialized" << std::endl; \
//         return find_impl<type>(k,default_value,found);  \
//     } \
//     template<> \
//     type Params::find(const std::string &k, std::string default_value, bool &found) const {  \
//         std::cout << "******* specialized" << std::endl; \
//         return find_impl<type>(k,default_value,found); \
//     } \
//     template <> \
//     type Params::find(const std::string &k, type default_value ) const { \
//         std::cout << "******* specialized" << std::endl; \
//         bool tmp; \
//         return find_impl<type>(k, default_value, tmp); \
//     } \
//     template <> \
//     type Params::find(const std::string &k, std::string default_value ) const { \
//         std::cout << "******* specialized" << std::endl; \
//         bool tmp; \
//         return find_impl<type>(k, default_value, tmp); \
//     } \
//     template <> \
//     type Params::find(const std::string &k) const {      \
//         std::cout << "******* specialized" << std::endl; \
//         bool tmp; \
//         type default_value = type(); \
//         return find_impl<type>(k, default_value, tmp); \
//     }
    

// SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(int32_t)
// SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(uint32_t)
// SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(int64_t)
// SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(uint64_t)
// SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(bool)
// SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(float)
// SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(double)
// SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(UnitAlgebra)

// // std::string has to be special cased because of signature conflicts
// //SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(std::string)
// template<>
// std::string Params::find<std::string>(const std::string &k, std::string default_value, bool &found) const {
//     std::cout << "******* specialized" << std::endl;
//     return find_impl<std::string>(k,default_value,found);
// }

std::map<std::string, uint32_t> Params::keyMap;
std::vector<std::string> Params::keyMapReverse;
Core::ThreadSafe::Spinlock Params::keyLock;
uint32_t Params::nextKeyID;
bool Params::g_verify_enabled = false;

}
