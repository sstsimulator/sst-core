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
//

#include "sst_config.h"
#include "sst/core/params.h"
#include "sst/core/unitAlgebra.h"

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
Params::print_all_params(std::ostream &os, const std::string& prefix) const
{
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        os << prefix << "key=" << keyMapReverse[i->first] << ", value=" << i->second << std::endl;
    }
}

void
Params::print_all_params(Output &out, const std::string& prefix) const
{
    for (const_iterator i = data.begin() ; i != data.end() ; ++i) {
        out.output("%s%s = %s\n", prefix.c_str(), keyMapReverse[i->first].c_str(), i->second.c_str());
    }
}


void
Params::insert(const std::string& key, const std::string& value, bool overwrite)
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
Params::getKey(const std::string& str) const
{
    std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(keyLock);
    auto i = keyMap.find(str);
    if ( i == keyMap.end() ) {
        return (uint32_t)-1;
    }
    return i->second;
}

uint32_t
Params::getKey(const std::string& str)
{
    std::lock_guard<SST::Core::ThreadSafe::Spinlock> lock(keyLock);
    auto i = keyMap.find(str);
    if ( i == keyMap.end() ) {
        uint32_t id = nextKeyID++;
        keyMap.insert(std::make_pair(str, id));
        keyMapReverse.push_back(str);
        assert(keyMapReverse.size() == nextKeyID);
        return id;
    }
    return i->second;
}


/**
   Private function for parsing string into array tokens.

 * Parses a string representing an array of tokens.  It is tailored to
 * the types strings you get when passing a python list as the param
 * string.  When you call addParam() on a python list in the input
 * file, it will call the str() function on the list, which creates a
 * string with the following format: [item1, item2, item3]

 * The format of the items depends on where they came from.  The
 * string for the items are generated by calling the repr() function
 * on them.  For strings, this means they will typically be enclosed
 * in single quotes.  It is possible that they end up enclosed in
 * double quotes if the string itself contains a single quote.  For
 * strings which contain both single and double quotes, the repr()
 * will create a single quoted string with all internal single quotes
 * escaped with '\'.  Most other items used in SST do not enclose the
 * string in quotes, though any string that contains a comma would
 * need to be enclosed in quotes, since the comma is the delimiter
 * character used.  This is not done automatically, so if you have
 * something that generates a commma in the string created by repr(),
 * you may need to create an array string manually.  Also, any string
 * that starts with a quote character, must end with the same quote
 * character.

 * The string passed into this function should have the [] already
 * removed from the string.  Tokens are generated by splitting the
 * string on commas that are not within quotes (double or single).
 * All whitespace at the beginning and end of a token is ignored
 * (unless inside quotes).  Once the tokens are generated, any quoted
 * string will have the front and back quotes removed.  The '\' for
 * any escaped quote of the same type as the front and back is also
 * removed.

 * Examples:

 * These will produce the same results:
 * [1, 2, 3, 4, 5]
 * ['1', '2', '3', '4', '5']

 * Examples of strings using double and/or single quotes:
 * 'This is "a" test'  ->  This is "a" test
 * "This is 'a' test" -> This is 'a' test
 * 'This "is \'a\'" test'  -> This "is 'a'" test
 * 'This "is \"a\"" test'  -> This "is \"a\"" test
*/
void
Params::getArrayTokens(const std::string& value, std::vector<std::string>& tokens) const
{
    bool in_quote = false;
    char quote_char = '\"';
    bool ignore_next_char = false;
    int start_index = -1;
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
            if ( value[i] == quote_char ) {
                in_quote = false;
            }
        }
        else {
            // In a token
            // If we find a comma, we're at the end of a token
            if ( value[i] == ',' ) {
                // Put token in vector
                tokens.push_back(value.substr(start_index,i-start_index));
                start_index = -1;
            }
            else if (value[i] == '\"' || value[i] == '\'') {
                in_quote = true;
                quote_char = value[i];
            }
        }
    }

    // Check to see if string ended in a token
    if ( start_index != -1 ) {
        tokens.push_back(value.substr(start_index));
    }

    // Now, clean-up tokens to remove an quotes from front and back.
    // Also, remove any whitespace on front and back and get rid of
    // any \ charaters that are escaping the quote type used around
    // the token.
    for ( auto& str : tokens ) {
        // Remove whitespace from end of token (whitespace was removed
        // from front during initial processing
        char test = str.back();
        while ( std::isspace(test) ) {
            str.pop_back();
            test = str.back();
        }

        // Remove leading and ending quotes and \ from escaped quotes
        // of same type as first and last
        if ( str.front() != '\"' && str.front() != '\'' ) {
            // No quotes, continue to next token
            continue;
        }
        char quote_char = str.front();
        // Check to see if the string is properly quotes front and
        // back and if it is, get rid of them.  If not, then it is an
        // error.
        if ( str.back() != quote_char ) {
            // ERROR
            std::string msg =
                "Params::find_array(): Invalid formatting: If token begins with a double or single quote, it must end with the same quote style: " + str;
            std::invalid_argument t(msg);
            throw t;
        }
        else {
            str = str.substr(1,str.size()-2);
        }

        // Remove '\' from espaced quote_chars
        for ( size_t i = 0; i < str.size(); ++i ) {
            // Check next character.  If it is quote_char, then remove
            // the '\'
            if ( str[i] == '\\' ) {
                if ( str[i+1] == quote_char ) str.erase(i,1);
            }
        }
    }
}


#if 0
 template<>
 uint32_t Params::find(const std::string& k) const
 {
     bool tmp;
     uint32_t default_value = uint32_t();
     return find(k, default_value, tmp);
 }


 #define SST_PARAMS_IMPLEMENT_TEMPLATE_SPECIALIZATION(type) \
     template<> \
     type Params::find(const std::string& k, type default_value, bool &found) const { \
         return find_impl<type>(k,default_value,found);  \
     } \
     template<> \
     type Params::find(const std::string& k, const std::string& default_value, bool &found) const {  \
         return find_impl<type>(k,default_value,found); \
     } \
     template <> \
     type Params::find(const std::string& k, type default_value ) const { \
         bool tmp; \
         return find_impl<type>(k, default_value, tmp); \
     } \
     template <> \
     type Params::find(const std::string& k, const std::string& default_value ) const { \
         bool tmp; \
         return find_impl<type>(k, default_value, tmp); \
     } \
     template <> \
     type Params::find(const std::string& k) const {      \
         bool tmp; \
         type default_value = type(); \
         return find_impl<type>(k, default_value, tmp); \
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
std::vector<std::string> Params::keyMapReverse;
Core::ThreadSafe::Spinlock Params::keyLock;
uint32_t Params::nextKeyID;
bool Params::g_verify_enabled = false;

}
