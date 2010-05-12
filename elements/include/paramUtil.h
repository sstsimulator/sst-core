// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



#ifndef _PARAMUTIL_H
#define _PARAMUTIL_H

#include <errno.h>
#include <sst/component.h>

inline long str2long( std::string str )
{
    long value = strtol( str.c_str(), NULL, 0 );
    if ( errno == EINVAL ) {
        _abort(XbarV2,"strtol( %s, NULL, 10\n", str.c_str());
    }
    return value;
}


inline void printParams( SST::Component::Params_t& params )
{
    SST::Component::Params_t::iterator it = params.begin();
    while( it != params.end() ) {
        printf("key=%s value=%s\n", it->first.c_str(),it->second.c_str());
        ++it;
    }
}

inline void findParams( std::string prefix, SST::Component::Params_t in,
                        SST::Component::Params_t& out )
{
    SST::Component::Params_t::iterator it = in.begin();
    while( it != in.end() )
    {
        if ( it->first.find( prefix.c_str(), 0, prefix.length() ) == 0 ) {
            std::string key = it->first.substr( prefix.length() );
            out[key] = it->second;
        }
        ++it;
    }
}

#endif
