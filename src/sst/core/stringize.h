// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_STRINGIZE_H
#define SST_CORE_STRINGIZE_H

#include <cctype>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <strings.h>
#include <vector>

namespace SST {

__attribute__((deprecated(
    "SST::to_string() is deprecated and will be removed in SST 13.  Please use std::to_string() instead"))) inline std::
    string
    to_string(double val)
{
    return std::to_string(val);
};

__attribute__((deprecated(
    "SST::to_string() is deprecated and will be removed in SST 13.  Please use std::to_string() instead"))) inline std::
    string
    to_string(float val)
{
    return std::to_string(val);
};

__attribute__((deprecated(
    "SST::to_string() is deprecated and will be removed in SST 13.  Please use std::to_string() instead"))) inline std::
    string
    to_string(int32_t val)
{
    return std::to_string(val);
};

__attribute__((deprecated(
    "SST::to_string() is deprecated and will be removed in SST 13.  Please use std::to_string() instead"))) inline std::
    string
    to_string(int64_t val)
{
    return std::to_string(val);
};

__attribute__((deprecated(
    "SST::to_string() is deprecated and will be removed in SST 13.  Please use std::to_string() instead"))) inline std::
    string
    to_string(uint32_t val)
{
    return std::to_string(val);
};

__attribute__((deprecated(
    "SST::to_string() is deprecated and will be removed in SST 13.  Please use std::to_string() instead"))) inline std::
    string
    to_string(uint64_t val)
{
    return std::to_string(val);
};

inline bool
strcasecmp(const std::string& s1, const std::string& s2)
{
    return !::strcasecmp(s1.c_str(), s2.c_str());
}

inline void
to_lower(std::string& s)
{
    for ( size_t i = 0; i < s.size(); i++ ) {
        s[i] = std::tolower(s[i]);
    }
}

inline void
trim(std::string& s)
{
    auto start = s.find_first_not_of(" \t\n\r\v\f");
    if ( start != 0 ) { s.replace(s.begin(), s.begin() + (start), ""); }
    auto end = s.find_last_not_of(" \t\n\r\v\f");
    if ( end != s.size() - 1 ) { s.replace(s.begin() + end + 1, s.end(), ""); }
}

inline void
tokenize(std::vector<std::string>& output, const std::string& input, const std::string& delim, bool trim_ws = false)
{
    size_t start = 0;
    size_t end   = input.find(delim);

    std::string token;
    while ( end != std::string::npos ) {
        token = input.substr(start, end - start);
        if ( trim_ws ) trim(token);
        output.push_back(token);
        start = end + delim.length();
        end   = input.find(delim, start);
    }

    token = input.substr(start, end);
    if ( trim_ws ) trim(token);
    output.push_back(token);
}

struct char_delimiter
{
    typedef std::string::const_iterator iter;
    const std::string                   delim;
    char_delimiter(const std::string& delim = " \t\v\f\n\r") : delim(delim) {}

    /**
     * @return pair<iter, iter> = <tok_end, next_tok>
     */
    void operator()(iter& first, iter last, std::string& token)
    {
        token.clear();

        /* Skip any leading separators */
        while ( first != last && delim.find(*first) != std::string::npos )
            ++first;

        while ( first != last && delim.find(*first) == std::string::npos )
            token += *first++;
    }
};

struct escaped_list_separator
{
    typedef std::string::const_iterator iter;
    std::string                         e;
    std::string                         q;
    std::string                         s;

    escaped_list_separator(
        const std::string& esc = "\\", const std::string& sep = ",", const std::string& quote = "\"") :
        e(esc),
        q(quote),
        s(sep)
    {}

    /**
     * @return pair<iter, iter> = <tok_end, next_tok>
     */
    void operator()(iter& first, iter last, std::string& token)
    {
        token.clear();

        bool inside_quotes = false;
        bool in_escape     = false;
        while ( first != last ) {
            char c = *first++;

            if ( in_escape ) {
                token += c;
                in_escape = false;
            }
            else if ( s.find(c) != std::string::npos && !inside_quotes ) {
                /* Separator found */
                break;
            }
            else if ( q.find(c) != std::string::npos ) {
                inside_quotes = !inside_quotes;
            }
            else if ( e.find(c) != std::string::npos ) {
                in_escape = true;
            }
            else {
                token += c;
            }
        }
    }
};

template <typename TokenizerFunc = char_delimiter>
class Tokenizer
{

    template <typename Func>
    struct token_iter
    {
        Func&                       f;
        std::string::const_iterator first;
        std::string::const_iterator last;
        std::string                 token;

    public:
        explicit token_iter(Func& f_, std::string::const_iterator& first_, std::string::const_iterator& last_) :
            f(f_),
            first(first_),
            last(last_)
        {
            f(first, last, token);
        }
        token_iter& operator++()
        {
            f(first, last, token);
            return *this;
        }
        token_iter operator++(int)
        {
            token_iter retval = *this;
            ++(*this);
            return retval;
        }
        bool operator==(token_iter other) const
        {
            return (first == other.first) && (last == other.last) && (token == other.token);
        }
        bool               operator!=(token_iter other) const { return !(*this == other); }
        const std::string& operator*() const { return token; }
        const std::string& operator->() const { return token; }

        using difference_type   = std::string;
        using value_type        = std::string;
        using pointer           = const std::string*;
        using reference         = const std::string&;
        using iterator_category = std::input_iterator_tag;
    };

    typedef token_iter<TokenizerFunc> iter;

public:
    typedef iter        iterator;
    typedef iter        const_iterator;
    typedef std::string value_type;

    iter begin() { return iter(f, first, last); }
    iter end() { return iter(f, last, last); }

    Tokenizer(const std::string& s, const TokenizerFunc& f = TokenizerFunc()) : first(s.begin()), last(s.end()), f(f) {}

private:
    std::string::const_iterator first, last;
    TokenizerFunc               f;
};


/**
   Creates a string using a vprintf like function call.  This function
   uses a dynamically allocated char array of size max_length to
   create the buffer to intialize the string.

   @param max_length Maximum length of string.  Anything past
   max_length will be truncated (null terminator is included in the
   length)

   @param format printf-like format string

   @param args va_list containing variable length argument list

   @return formatted string, potentially truncated at length max_length - 1
 */
std::string vformat_string(size_t max_length, const char* format, va_list args);

/**
   Creates a string using a printf like function call.  This function
   uses a compile time allocated char array of length 256 to create
   the buffer to intialize the string.  If this is not long enough, it
   will dynamically allocate an array that is just big enough to
   create the buffer to initialize the string.  No truncation will
   occur.

   @param format printf-like format string

   @param args va_list containing variable length argument list

   @return formatted string
 */
std::string vformat_string(const char* format, va_list args);

/**
   Creates a string using a printf like function call.  This function
   uses a dynamically allocated char array of size max_length to
   create the buffer to intialize the string.

   @param max_length Maximum length of string.  Anything past
   max_length will be truncated (null terminator is included in the
   length)

   @param format printf-like format string

   @param ... arguments for format string

   @return formatted string, potentially truncated at length max_length - 1
 */
std::string format_string(size_t max_length, const char* format, ...) __attribute__((format(printf, 2, 3)));

/**
   Creates a string using a printf like function call.  This function
   uses a compile time allocated char array of length 256 to create
   the buffer to intialize the string.  If this is not long enough, it
   will dynamically allocate an array that is just big enough to
   create the buffer to initialize the string.  No truncation will
   occur.

   @param format printf-like format string

   @param ... arguments for format string

   @return formatted string
 */
std::string format_string(const char* format, ...) __attribute__((format(printf, 1, 2)));

} // namespace SST

#endif // SST_CORE_STRINGIZE_H
