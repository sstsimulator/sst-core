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

#ifndef _H_SST_CORE_STRINGIZE
#define _H_SST_CORE_STRINGIZE

#include <string>
#include <strings.h>
#include <cinttypes>
#include <cctype>

namespace SST {

inline std::string to_string(double val) {
    char buffer[256];
    sprintf(buffer, "%f", val);

    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(float val) {
    char buffer[256];
    sprintf(buffer, "%f", val);

    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(int32_t val) {
    char buffer[256];
    sprintf(buffer, "%" PRId32, val);

    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(int64_t val) {
    char buffer[256];
    sprintf(buffer, "%" PRId64, val);

    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(uint32_t val) {
    char buffer[256];
    sprintf(buffer, "%" PRIu32, val);

    std::string buffer_str(buffer);
    return buffer_str;
};

inline std::string to_string(uint64_t val) {
    char buffer[256];
    sprintf(buffer, "%" PRIu64, val);

    std::string buffer_str(buffer);
    return buffer_str;
};


inline bool strcasecmp(const std::string &s1, const std::string &s2) {
    return !::strcasecmp(s1.c_str(), s2.c_str());
}

inline void to_lower(std::string &s) {
    for ( size_t i = 0 ; i < s.size() ; i++ ) {
        s[i] = std::tolower(s[i]);
    }
}


inline void trim(std::string &s) {
    auto start = s.find_first_not_of(" \t\n\r\v\f");
    if ( start != 0 ) {
        s.replace(s.begin(), s.begin() + (start), "");
    }
    auto end = s.find_last_not_of(" \t\n\r\v\f");
    if ( end != s.size() -1 ) {
        s.replace(s.begin() + end + 1, s.end(), "");
    }
}



struct char_delimiter {
    typedef std::string::const_iterator iter;
    const std::string delim;
    char_delimiter(const std::string& delim = " \t\v\f\n\r")
        : delim(delim) { }

    /**
     * @return pair<iter, iter> = <tok_end, next_tok>
     */
    void operator()(iter &first, iter last, std::string &token) {
        token.clear();

        /* Skip any leading separators */
        while ( first != last && delim.find(*first) != std::string::npos )
            ++first;

        while ( first != last && delim.find(*first) == std::string::npos )
            token += *first++;
    }

};



struct escaped_list_separator {
    typedef std::string::const_iterator iter;
    std::string e;
    std::string q;
    std::string s;

    escaped_list_separator(const std::string &esc="\\", const std::string &sep=",", const std::string &quote="\"")
        : e(esc), q(quote), s(sep) { }


    /**
     * @return pair<iter, iter> = <tok_end, next_tok>
     */
    void operator()(iter &first, iter last, std::string &token) {
        token.clear();

        bool inside_quotes = false;
        bool in_escape = false;
        while ( first != last ) {
            char c = *first++;

            if ( in_escape ) {
                token += c;
                in_escape = false;
            } else if ( s.find(c) != std::string::npos && !inside_quotes ) {
                /* Separator found */
                break;
            } else if ( q.find(c) != std::string::npos ) {
                inside_quotes = !inside_quotes;
            } else if ( e.find(c) != std::string::npos ) {
                in_escape = true;
            } else {
                token += c;
            }
        }
    }

};



template< typename TokenizerFunc = char_delimiter >
class Tokenizer {

    template <typename Func>
    struct token_iter
        : public std::iterator< std::input_iterator_tag, std::string >
    {
        Func &f;
        std::string::const_iterator first;
        std::string::const_iterator last;
        std::string token;
    public:
        explicit token_iter(Func& f_, std::string::const_iterator& first_, std::string::const_iterator& last_)
            : f(f_), first(first_), last(last_)
        {
            f(first, last, token);
        }
        token_iter& operator++() { f(first, last, token); return *this; }
        token_iter  operator++(int) {token_iter retval = *this; ++(*this); return retval;}
        bool operator==(token_iter other) const { return (first == other.first) && (last == other.last) && (token == other.token); }
        bool operator!=(token_iter other) const { return !(*this == other); }
        const std::string& operator*() const { return token; }
        const std::string& operator->() const { return token; }
    };



    typedef token_iter<TokenizerFunc> iter;
public:
    typedef iter iterator;
    typedef iter const_iterator;
    typedef std::string value_type;

    iter begin() { return iter(f, first, last); }
    iter end() { return iter(f, last, last); }

    Tokenizer(const std::string &s, const TokenizerFunc& f = TokenizerFunc())
        : first(s.begin()), last(s.end()), f(f) { }

private:
    std::string::const_iterator first, last;
    TokenizerFunc f;
};

}

#endif
