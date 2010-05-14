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


// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "SS_network.h"

#include <paramUtil.h>
#include <sst/debug.h>

Network::Network( SST::Component::Params_t params )
{
    if ( params.find( "xDimSize" ) == params.end() ) {
        _abort(Network,"couldn't find xDimSize\n" );
    } 
    _xDimSize = str2long( params[ "xDimSize" ] );

    if ( params.find( "yDimSize" ) == params.end() ) {
        _abort(Network,"couldn't find yDimSize\n" );
    } 
    _yDimSize = str2long( params[ "yDimSize" ] );

    if ( params.find( "zDimSize" ) == params.end() ) {
        _abort(Network,"couldn't find zDimSize\n" );
    } 
    _zDimSize = str2long( params[ "zDimSize" ] );

    _size = _xDimSize*_yDimSize*_zDimSize;
#if 0
    _DBG(0,"size=%d xdim=%d ydim=%d xdim=%d\n",
           _size,_xDimSize,_yDimSize,_zDimSize);
#endif
}
