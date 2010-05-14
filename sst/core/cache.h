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


#ifndef SST_CACHE_H
#define SST_CACHE_H

#include <sst/core/debug.h>
#include <sst/core/boost.h>

namespace SST {

#define _CACHE_DBG( fmt, args...) __DBG( DBG_CACHE, Cache, fmt, ## args ) 

template <typename KeyT, typename DataT>
class Cache {

        static const int num_sets = 1024;
        static const int set_size = 1;
        static const unsigned int mask = num_sets - 1;

    public:
        Cache( ) 
        {
            _CACHE_DBG("new\n");
            for ( int i = 0; i < num_sets * set_size; i++ ) {
                valid[ i] = false;   
		data[i] = DataT();
            }
        }

        void Inject( KeyT key, DataT _data ) {
            _CACHE_DBG( "key=%ld data=%#lx\n", key, (unsigned long) data ); 
            data[ key&mask ]  = _data;
            valid[ key&mask ] = true;
            keys[ key&mask ]  = key;
            _CACHE_DBG( "done\n" ); 
        }

        void Invalidate( KeyT key ) {
            _CACHE_DBG( "key=%ld\n", key ); 
            valid[ key&mask ] = false;
        }

        bool Read( KeyT key, DataT* _data )  {
            _CACHE_DBG( "key=%ld\n", key ); 
            *_data = data[ key&mask ]; 
            return valid[ key&mask ] && keys[ key&mask ] == key;
        }
    private:

#if WANT_CHECKPOINT_SUPPORT

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version )
	{
	    _AR_DBG(Cache,"\n");
	    // zero invalid data entries so as not to confuse the
	    // serializer
	    for ( int i = 0; i < num_sets * set_size; i++ ) {
		if (!valid[i]) {
		    data[i] = DataT();
		}
	    }
	    _AR_DBG(Cache," Begin\n");
	    ar & BOOST_SERIALIZATION_NVP( keys );
	    ar & BOOST_SERIALIZATION_NVP( data );
	    ar & BOOST_SERIALIZATION_NVP( valid );
	    _AR_DBG(Cache," Done\n");
	}

#endif // WANT_CHECKPOINT_SUPPORT
	


        KeyT  keys[ num_sets * set_size ];
        DataT data[ num_sets * set_size ];
        bool  valid[ num_sets * set_size ];
};

}

#endif // SST_CACHE_H
