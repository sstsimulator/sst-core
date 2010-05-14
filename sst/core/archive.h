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


#ifndef SST_ARCHIVE_H
#define SST_ARCHIVE_H

#if WANT_CHECKPOINT_SUPPORT

#include <fstream>
#include <iostream>
#include <sst/boost.h>
#include <sst/simulation.h>

#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace SST {
namespace Archive {

typedef enum { UNKNOWN, XML, TEXT, BIN } Type_t; 

inline Type_t Str2Type( std::string name ) {
    if ( ! name.compare("xml") ) return XML; 
    if ( ! name.compare("text") ) return TEXT; 
    if ( ! name.compare("bin") ) return BIN; 
    return UNKNOWN;
}

static inline std::string Type2Str( Type_t type ) {
    switch ( type ) {
        case XML:
            return "xml";
            break;
        case TEXT:
            return "text";
            break;
        case BIN:
            return "bin";
            break;
        case UNKNOWN:
            break;
    }
    return "unknown";
}

template <typename Type >
static int xml_save( Type & object, std::ofstream& ofs  )
{
    boost::archive::xml_oarchive oa( ofs );
    oa << BOOST_SERIALIZATION_NVP( object );
    return 0;
}

template <typename Type >
static int xml_load( Type & object, std::ifstream& ifs  )
{
    boost::archive::xml_iarchive ia( ifs );
    ia >> BOOST_SERIALIZATION_NVP( object );
    return 0;
}

template <typename Type >
static int text_save( Type & object, std::ofstream& ofs  )
{
    boost::archive::text_oarchive oa( ofs );
    oa << BOOST_SERIALIZATION_NVP( object );
    return 0;
}

template <typename Type >
static int text_load( Type & object, std::ifstream& ifs  )
{
    boost::archive::text_iarchive ia( ifs );
    ia >> BOOST_SERIALIZATION_NVP( object );
    return 0;
}

template <typename Type >
static int binary_save( Type & object, std::ofstream& ofs  )
{
    boost::archive::binary_oarchive oa( ofs );
    oa << BOOST_SERIALIZATION_NVP( object );
    return 0;
}

template <typename Type >
static int binary_load( Type & object, std::ifstream& ifs  )
{
    boost::archive::binary_iarchive ia( ifs );
    ia >> BOOST_SERIALIZATION_NVP( object );
    return 0;
}

template <typename Type >
int Save( Type object, Type_t & type, std::string filename  )
{
    int ret = -1;

    filename.append( "." );
    filename.append( Type2Str( type ) );
    std::ofstream ofs( filename.c_str() );

    assert( ofs.good() );

    switch ( type ) {
        case XML:
            ret =xml_save( object, ofs );
            break;
        case TEXT:
            ret = text_save( object, ofs );
            break;
        case BIN:
            ret = binary_save( object, ofs );
            break;
        case UNKNOWN:
            break;
    } 
    ofs.close();
    return ret;
}

template <typename Type >
int Load( Type & object, Type_t type, std::string filename  )
{
    int ret = -1;

    filename.append( "." );
    filename.append( Type2Str( type ) );
    std::ifstream ifs( filename.c_str() );

    assert( ifs.good() );

    switch ( type ) {
        case XML:
            ret = xml_load( object, ifs );
            break;
        case TEXT:
            ret = text_load( object, ifs );
            break;
        case BIN:
            ret = binary_load( object, ifs );
            break;
        case UNKNOWN:
            break;
    }
    ifs.close();
    return ret;
}
} //namespace Archive

} //namespace SST

#endif

#endif // SST_ARCHIVE_H
