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


#ifndef _SST_QUEUE_H
#define _SST_QUEUE_H

#include <deque>
#include <map>
#include <sst/core/boost.h>
#include <sst/core/cache.h>
#include <sst/core/pool.h>

namespace SST {

#define _EQ_DBG( fmt, args...) __DBG( DBG_QUEUE, Queue, fmt, ## args )

template <typename KeyT, typename DataT>
class Queue {
    private:
        typedef std::deque<DataT> queue_t; 

    public:
        void insert( KeyT key, DataT data ) {
	    
            _EQ_DBG("this=%p start key=%lu\n",this,key);
            queue_t *q_ptr;
            if ( ! cache.Read( key, &q_ptr ) ) {
                if ( map.find( key ) == map.end()  ) {
                    map[key] = q_ptr = q_pool.Alloc(); 
                    _EQ_DBG("new q_ptr=%p\n",q_ptr);
                } else {
                    q_ptr = map[key];
                }
                cache.Inject( key, q_ptr );
            }
            q_ptr->push_front(data);
            _EQ_DBG("q_ptr=%p size=%lu\n",q_ptr,(unsigned long)q_ptr->size());
        }

        void pop()   { 
            _EQ_DBG("start %ld\n",(unsigned long)map.begin()->second->size());
            map.begin()->second->pop_front();
            if ( map.begin()->second->empty() ) {
                _EQ_DBG("empty\n");
                q_pool.Dealloc( map.begin()->second );
                cache.Invalidate( map.begin()->first );
                map.erase( map.begin() );
            }
            _EQ_DBG("done\n");
        }

        DataT top()  { return map.begin()->second->front(); }
        KeyT key()   { return map.begin()->first; }
        bool empty() { return map.empty(); }

    private:
        std::map<KeyT,queue_t*> map;
        Cache<KeyT,queue_t*>    cache;

        // don't serialize
        Pool<queue_t>           q_pool;

#if WANT_CHECKPOINT_SUPPORT
	
	BOOST_SERIALIZE {
            _AR_DBG(Queue,"start\n");
	    printf("Serializing: map\n");
            ar & BOOST_SERIALIZATION_NVP(map);
	    printf("Serializing: cache\n");
            ar & BOOST_SERIALIZATION_NVP(cache);
            _AR_DBG(Queue,"done\n");
        }

#endif

};

} // namespace SST

#endif
