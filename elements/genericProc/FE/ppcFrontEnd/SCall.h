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


#ifndef SCALL_H
#define SCALL_H

//:System Caller
//
// Several specialized template functions called 'SCall' assist in
// calling common system calls in the ppc front end. They generally
// take a function pointer and list of arguments, and then call the
// function.
//
//!SEC:ppcFront
template <class T, class S, int N>
struct SCall {
  int doIt(T func, S a, int b=0, int c=0, int d=0);
};

//!IGNORE:
template <class T, class S>
struct SCall<T,S,-1> {
  int doIt(T func, S a, int b=0, int c=0, int d=0) {
    return func();
  }
};

//!IGNORE:
template <class T, class S>
struct SCall<T,S,0> {
  int doIt(T func, S a, int b=0, int c=0, int d=0) {
    return func(a);
  }
};

//!IGNORE:
template <class T, class S>
struct SCall<T,S,1> {
  int doIt(T func, S a, int b=0, int c=0, int d=0) {
    return func(a, b);
  }
};

//!IGNORE:
template <class T, class S>
struct SCall<T,S,2> {
  int doIt(T func, S a, int b=0, int c=0, int d=0) {
    return func(a, b, c);
  }
};

#endif
