// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2010, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sst/component.h"

#include "thread.h"
#include "fe_debug.h"
#include "ppcFrontEnd/ppcFront.h"

//#include "configuration.h"
//#include "ssFrontEnd/ssFront.h"
//#include "traceFrontEnd/traceFront.h"

//#ifdef __APPLE__
#include "FE/ppcFrontEnd/ppcFront.h"
//#endif

//: Access the first thread to start the simulation
thread* threadSource::getFirstThread(unsigned int n) {

  INFO("Getting first thread %d\n", n);

  assert(!firstThreads.empty());
  if (n < firstThreads.size()) {
    return(firstThreads[n]);
  } else {
    return 0;
  }
}

//: Initialize the front end
//
// A switch to the underlying front end which begins intialization. 
void threadSource::init(processor *p, SST::Component::Params_t& paramsC) {
  proc = p;
  /*string fe = configuration::getStrValue(cfgstr + ":frontEnd");
  
  if (fe == "ss") 
    {
      firstThreads = ssThread::init(cfgstr);
    }
  else if (fe == "ppc") 
  {*/
  firstThreads = ppcThread::init(p, paramsC);
      /*} 
  else if (fe == "trace") 
    {
      firstThreads = traceThread::init(cfgstr);
    } 
  else 
    {
      fprintf(stderr, "thread::init(): unknown front end: %s\n", fe.c_str());
      exit(-1);
      }*/
}

//: Deallocate a dead thread
//
// Requests that the front end remove a thread.
void threadSource::deleteThread(thread *t)
{
  //t->recordMemStat();
  ppcThread::deleteThread(t);

  /*static bool inited = 0;
  static string fe;
  if (!inited) {    
    fe = configuration::getStrValue(cfgstr + ":frontEnd");
    inited = 1;
    if (_config_error) {
      fe = configuration::getStrValue(":frontEnd");
    }
  }   

  t->recordMemStat();

    if (fe == "ss") 
    {
        ssThread::deleteThread(t);
    } 
    else if (fe == "ppc") 
    {
        ppcThread::deleteThread(cfgstr,t);
    } 
    else if (fe == "trace") 
    {
        traceThread::deleteThread(t);
    } 
    else 
    {
        fprintf(stderr, "thread::deleteThread(): unknown front end: %s\n", 
                fe.c_str());
        exit(-1);
	}*/
}
