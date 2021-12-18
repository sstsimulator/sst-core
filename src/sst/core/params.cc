// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include "sst_config.h"

#include "sst/core/params.h"

#include "sst/core/unitAlgebra.h"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#define SET_NAME_KEYWORD "GLOBAL_SET_NAME"

namespace SST {

const std::string&
Params::getString(const std::string& name, bool& found) const
{
    static std::string empty;
    for ( auto map : data ) {
        auto value = map->find(getKey(name));
        if ( value != map->end() ) {
            found = true;
            return value->second;
        }
    }
    found = false;
    return empty;
}

size_t
Params::size() const
{
    return getKeys().size();
}

bool
Params::empty() const
{
    return getKeys().empty();
}

Params::Params() : my_data(), verify_enabled(true)
{
    data.push_back(&my_data);
}

Params::Params(const Params& old) :
    my_data(old.my_data),
    data(old.data),
    allowedKeys(old.allowedKeys),
    verify_enabled(old.verify_enabled)
{
    data[0] = &my_data;
}

Params&
Params::operator=(const Params& old)
{
    my_data        = old.my_data;
    data           = old.data;
    data[0]        = &my_data;
    verify_enabled = old.verify_enabled;
    allowedKeys    = old.allowedKeys;
    return *this;
}

void
Params::clear()
{
    my_data.clear();
    data.clear();
    data.push_back(&my_data);
}

size_t
Params::count(const key_type& k) const
{
    int key = getKey(k);
    for ( auto map : data ) {
        size_t count = map->count(key);
        if ( count > 0 ) return count;
    }
    return 0;
}

void
Params::print_all_params(std::ostream& os, const std::string& prefix) const
{
    int level = 0;
    for ( auto map : data ) {
        if ( level == 0 ) {
            if ( !map->empty() ) os << "Local params:" << std::endl;
            level++;
        }
        else if ( level == 1 ) {
            os << "Global params:" << std::endl;
            level++;
        }

        for ( auto value : *map ) {
            os << "  " << prefix << "key=" << keyMapReverse[value.first] << ", value=" << value.second << std::endl;
        }
    }
}

void
Params::print_all_params(Output& out, const std::string& prefix) const
{
    int level = 0;
    for ( auto map : data ) {
        if ( level == 0 ) {
            if ( !map->empty() ) out.output("Local params:\n");
            level++;
        }
        else if ( level == 1 ) {
            out.output("Global params:\n");
            level++;
        }

        for ( auto value : *map ) {
            out.output("%s%s = %s\n", prefix.c_str(), keyMapReverse[value.first].c_str(), value.second.c_str());
        }
    }
}

void
Params::insert(const std::string& key, const std::string& value, bool overwrite)
{
    if ( overwrite ) { my_data[getKey(key)] = value; }
    else {
        uint32_t id = getKey(key);
        my_data.insert(std::make_pair(id, value));
    }
}

void
Params::insert(const Params& params)
{
    my_data.insert(params.my_data.begin(), params.my_data.end());
    for ( size_t i = 1; i < params.data.size(); ++i ) {
        bool already_there = false;
        for ( auto x : data ) {
            if ( params.data[i] == x ) already_there = true;
        }
        if ( !already_there ) data.push_back(params.data[i]);
    }
}

std::set<std::string>
Params::getKeys() const
{
    std::set<std::string> ret;
    for ( auto map : data ) {
        for ( auto value : *map ) {
            ret.insert(keyMapReverse[value.first]);
        }
    }
    return ret;
}

Params
Params::find_scoped_params(const std::string& prefix, const char* delims) const
{
    int    num_delims = ::strlen(delims);
    Params ret;
    ret.enableVerify(false);
    for ( auto map : data ) {
        for ( auto value : *map ) {
            auto&       fullKeyName = keyMapReverse[value.first];
            std::string key         = fullKeyName.substr(0, prefix.length());
            auto        start       = prefix.length();
            if ( key == prefix ) {
                char next         = fullKeyName[start];
                bool delimMatches = false;
                for ( int i = 0; i < num_delims; ++i ) {
                    if ( next == delims[i] ) {
                        delimMatches = true;
                        break;
                    }
                }
                if ( delimMatches ) { ret.insert(keyMapReverse[value.first].substr(start + 1), value.second); }
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

    for ( auto map : data ) {
        for ( auto value : *map ) {
            std::string key = keyMapReverse[value.first].substr(0, prefix.length());
            if ( key == prefix ) { ret.insert(keyMapReverse[value.first].substr(prefix.length()), value.second); }
        }
    }
    ret.allowedKeys = allowedKeys;
    ret.enableVerify(verify_enabled);

    return ret;
}

Params
Params::get_scoped_params(const std::string& scope) const
{
    Params ret;
    ret.enableVerify(false);

    std::string prefix = scope + ".";
    for ( auto map : data ) {
        for ( auto value : *map ) {
            std::string key = keyMapReverse[value.first].substr(0, prefix.length());
            if ( key == prefix ) { ret.insert(keyMapReverse[value.first].substr(prefix.length()), value.second); }
        }
    }
    ret.allowedKeys = allowedKeys;
    ret.enableVerify(verify_enabled);

    return ret;
}

bool
Params::contains(const key_type& k) const
{
    for ( auto map : data ) {
        if ( map->find(getKey(k)) != map->end() ) return true;
    }
    return false;
}

void
Params::pushAllowedKeys(const KeySet_t& keys)
{
    allowedKeys.push_back(keys);
}

void
Params::popAllowedKeys()
{
    allowedKeys.pop_back();
}

void
#ifdef USE_PARAM_WARNINGS
Params::verifyKey(const key_type& k) const
#else
Params::verifyKey(const key_type& UNUSED(k)) const
#endif
{
#ifdef USE_PARAM_WARNINGS
    if ( !g_verify_enabled || !verify_enabled ) return;

    for ( std::vector<KeySet_t>::const_reverse_iterator ri = allowedKeys.rbegin(); ri != allowedKeys.rend(); ++ri ) {
        if ( ri->find(k) != ri->end() ) return;
    }

    SST::Output outXX("ParamWarning: ", 0, 0, Output::STDERR);
    outXX.output(CALL_INFO, "Warning: Parameter \"%s\" is undocumented.\n", k.c_str());
#endif
}

void
Params::verifyParam(const key_type& k) const
{
    verifyKey(k);
}

const std::string&
Params::getParamName(uint32_t id)
{
    return keyMapReverse[id];
}

void
Params::serialize_order(SST::Core::Serialization::serializer& ser)
{
    ser&                     my_data;
    // Serialize global params
    std::vector<std::string> globals;
    switch ( ser.mode() ) {
    case SST::Core::Serialization::serializer::PACK:
    case SST::Core::Serialization::serializer::SIZER:
        for ( size_t i = 1; i < data.size(); ++i ) {
            globals.push_back((*data[i])[0]);
        }
        ser& globals;
        break;
    case SST::Core::Serialization::serializer::UNPACK:
        ser& globals;
        for ( auto x : globals )
            data.push_back(&global_params[x]);
        break;
    }
}

uint32_t
Params::getKey(const std::string& str)
{
    std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(keyLock);
    auto                                             i = keyMap.find(str);
    if ( i == keyMap.end() ) {
        uint32_t id = nextKeyID++;
        keyMap.insert(std::make_pair(str, id));
        keyMapReverse.push_back(str);
        // ID 0 is reserved for holding metadata
        assert(keyMapReverse.size() == nextKeyID);
        return id;
    }
    return i->second;
}

void
Params::addGlobalParamSet(const std::string& set)
{
    data.push_back(&global_params[set]);
}

void
Params::insert_global(const std::string& global_key, const std::string& key, const std::string& value, bool overwrite)
{
    std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(globalLock);
    if ( global_params.count(global_key) == 0 ) { global_params[global_key][0] = global_key; }
    if ( overwrite ) { global_params[global_key][getKey(key)] = value; }
    else {
        global_params[global_key].insert(std::make_pair(getKey(key), value));
    }
}

void
Params::getDelimitedTokens(const std::string& value, char delim, std::vector<std::string>& tokens) const
{
    bool in_quote         = false;
    char quote_char       = '\"';
    bool ignore_next_char = false;
    int  start_index      = -1;
    for ( size_t i = 0; i < value.size(); ++i ) {
        if ( ignore_next_char ) {
            ignore_next_char = false;
            continue;
        }
        if ( start_index == -1 ) {
            // No longer in a token, check to see if we're
            // starting a new one.

            // Skip whitespace before starting token (unless in
            // quotes
            if ( std::isspace(value[i]) ) continue;

            start_index = i;
        }

        if ( in_quote ) {
            // Look for end of quote, otherwise, just skip
            // character
            if ( value[i] == '\\' ) {
                ignore_next_char = true;
                continue;
            }
            if ( value[i] == quote_char ) { in_quote = false; }
        }
        else {
            // In a token
            // If we find a delimiter, we're at the end of a token
            if ( value[i] == delim ) {
                // Put token in vector
                tokens.push_back(value.substr(start_index, i - start_index));
                start_index = -1;
            }
            else if ( value[i] == '\"' || value[i] == '\'' ) {
                in_quote   = true;
                quote_char = value[i];
            }
        }
    }
    // Check to see if string ended in a token
    if ( start_index != -1 ) { tokens.push_back(value.substr(start_index)); }
}

void
Params::cleanToken(std::string& token) const
{
    // Remove whitespace from end of token (whitespace was removed
    // from front during initial processing
    char test = token.back();
    while ( std::isspace(test) ) {
        token.pop_back();
        test = token.back();
    }

    // Remove leading and ending quotes and \ from escaped quotes
    // of same type as first and last
    if ( token.front() != '\"' && token.front() != '\'' ) {
        // No quotes, continue to next token
        return;
    }
    char quote_char = token.front();
    // Check to see if the string is properly quotes front and
    // back and if it is, get rid of them.  If not, then it is an
    // error.
    if ( token.back() != quote_char ) {
        // ERROR
        std::string msg = "Invalid formatting: If token begins with a double or single "
                          "quote, it must end with the same quote style: " +
                          token;
        throw std::invalid_argument(msg);
    }
    else {
        token = token.substr(1, token.size() - 2);
    }

    // Remove '\' from espaced quote_chars
    for ( size_t i = 0; i < token.size(); ++i ) {
        // Check next character.  If it is quote_char, then remove
        // the '\'
        if ( token[i] == '\\' ) {
            if ( token[i + 1] == quote_char ) token.erase(i, 1);
        }
    }
}

std::map<std::string, std::string>
Params::getGlobalParamSet(const std::string& name)
{
    std::map<std::string, std::string> ret;
    auto                               it = global_params.find(name);
    if ( it == global_params.end() ) return ret;

    for ( auto x : it->second ) {
        ret[getParamName(x.first)] = x.second;
    }
    return ret;
}

std::vector<std::string>
Params::getGlobalParamSetNames()
{
    std::vector<std::string> ret;
    ret.reserve(global_params.size());
    for ( auto x : global_params ) {
        ret.push_back(x.first);
    }
    return ret;
}


std::vector<std::string>
Params::getLocalKeys() const
{
    std::vector<std::string> ret;
    ret.reserve(data[0]->size());
    for ( auto x : *data[0] ) {
        ret.push_back(getParamName(x.first));
    }
    return ret;
}


std::vector<std::string>
Params::getSubscribedGlobalParamSets() const
{
    std::vector<std::string> ret;
    ret.reserve((data.size() - 1));
    // Skip the first item because those are local params
    for ( size_t i = 1; i < data.size(); ++i ) {
        // To get <set_name> key, need to use keyID 0
        ret.push_back((*data[i])[0]);
    }
    return ret;
}

#if 0
 template<>
 uint32_t Params::find(const std::string& k) const
 {
     bool tmp;
     uint32_t default_value = uint32_t();
     return find(k, default_value, tmp);
 }

#define SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(type)                                       \
    template <>                                                                                  \
    type Params::find(const std::string& k, type default_value, bool& found) const               \
    {                                                                                            \
        return find_impl<type>(k, default_value, found);                                         \
    }                                                                                            \
    template <>                                                                                  \
    type Params::find(const std::string& k, const std::string& default_value, bool& found) const \
    {                                                                                            \
        return find_impl<type>(k, default_value, found);                                         \
    }                                                                                            \
    template <>                                                                                  \
    type Params::find(const std::string& k, type default_value) const                            \
    {                                                                                            \
        bool tmp;                                                                                \
        return find_impl<type>(k, default_value, tmp);                                           \
    }                                                                                            \
    template <>                                                                                  \
    type Params::find(const std::string& k, const std::string& default_value) const              \
    {                                                                                            \
        bool tmp;                                                                                \
        return find_impl<type>(k, default_value, tmp);                                           \
    }                                                                                            \
    template <>                                                                                  \
    type Params::find(const std::string& k) const                                                \
    {                                                                                            \
        bool tmp;                                                                                \
        type default_value = type();                                                             \
        return find_impl<type>(k, default_value, tmp);                                           \
    }


 SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(int32_t)
 SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(uint32_t)
 SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(int64_t)
 SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(uint64_t)
 SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(bool)
 SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(float)
 SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(double)
 SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(UnitAlgebra)

 // std::string has to be special cased because of signature conflicts
 //SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(std::string)
 template<>
 std::string Params::find<std::string>(const std::string& k, const std::string& default_value, bool &found) const {
     return find_impl<std::string>(k,default_value,found);
 }
#endif

std::map<std::string, uint32_t> Params::keyMap;
// Index 0 in params is used for set name
std::vector<std::string>        Params::keyMapReverse({ "<set_name>" });
uint32_t                        Params::nextKeyID = 1;
Core::ThreadSafe::Spinlock      Params::keyLock;
Core::ThreadSafe::Spinlock      Params::globalLock;
// ID 0 is reserved for holding metadata
bool                            Params::g_verify_enabled = false;

std::map<std::string, std::map<uint32_t, std::string>> Params::global_params;

} // namespace SST
