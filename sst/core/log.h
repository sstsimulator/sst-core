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


#ifndef SST_CORE_LOG_H
#define SST_CORE_LOG_H

#include <cstdarg>
#include <stdio.h>
#include <string>

namespace SST { 

template < int ENABLE = 1 >
class Log {
    public:
        Log( std::string prefix = "", bool enable = true ) :
            m_prefix( prefix ), m_enabled( enable )
        {}
    
        void enable() {
            m_enabled = true;
        }
        void disable() {
            m_enabled = false;
        }

        void prepend( std::string str ) {
            m_prefix = str + m_prefix; 
        }

        inline void write( const std::string fmt, ... )
        {
            if ( ENABLE ) {

                if ( ! m_enabled ) return;

                printf( "%s", m_prefix.c_str() );
                va_list ap;
                va_start( ap,fmt );
                vprintf( fmt.c_str(), ap ); 
                va_end( ap);
            }
        }

    private:
        std::string m_prefix;
        bool        m_enabled;
};
} // namespace SST

#endif //SST_CORE_LOG_H
