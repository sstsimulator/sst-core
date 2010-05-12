// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2007, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _POOL_H_
#define _POOL_H_

#include <vector>
using std::vector;
#include "sst/boost.h"

//: Abstract Pool class
//
// Templated, and gives access and return pointers to templated class
//!SEC:Level_1
template <class T>
class pool 
{
#if WANT_CHECKPOINT_SUPPORT    
BOOST_SERIALIZE {
    ar & BOOST_SERIALIZATION_NVP(_pool);
    ar & BOOST_SERIALIZATION_NVP(num);
  }
#endif
public:
  pool( int size = 10 );
  ~pool( void );
  inline T* getItem( void );
  inline void returnItem( T* item );
  
private:
  int num;
  //: The actual pool
  //
  // simple vector
  vector<T*> _pool;
};

//: Constructor
//
//!in: size: The initial size of the pool
template <class T>
pool<T>::pool( const int size )
{
  /* Make it a bit larger than probably needed. */
  _pool.reserve(size * 2);
  /* Fill it up */
  for (int i = 0 ; i < size ; ++i ) {
    _pool.push_back(new T);
  }
  num = size;
}

//: Destructor
//
// Temporarily disabled, because I'm not sure how to handle the
// "looping" - when instrPool tries to delete an instruction, the
// instruction tries to add its singleInstructions to the
// singInstPool, which has already been deleted...
template <class T>
pool<T>::~pool( void )
{
  /*for (vector<T*>::iterator i = _pool.begin();
       i != _pool.end();
       ++i) {
       delqete *i;
   }*/
}

//: Returns an item from the pool
//
// If its in the pool, it returns it, otherwise it new() another
template <class T>
T* pool<T>::getItem( void )
{
  T* ret;
  //if (_pool.empty() == 0) {
  if (num > 0) {
    num--;
    ret = _pool.back();
    _pool.pop_back();
  } else {
    ret = new T;
    num++;
    _pool.push_back(new T);
  }
  if (ret == 0) {
    fprintf(stderr,"Pool Error (returning 0)\n");
    exit(-1);
  }
  return ret;
}

//: Returns and item to the pool
//
//!in: item: pointer to item to return
template<class T>
void pool<T>::returnItem( T* item )
{
  if (item) {
    num++;
    _pool.push_back(item);
  }
}

#endif /* _POOL_H_ */
