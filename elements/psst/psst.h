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

#ifndef _PSST_H
/*----------------------------------------------------------+----------------\
| Psst                                                      | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| Processor component for SST.                                               |
\---------------------------------------------------------------------------*/
#define _PSST_H
#include <vector>
#include <utility>
#include <string>

#include <sst/eventFunctor.h>
#include <sst/component.h>
#include <sst/link.h>

#include "models/model.h"

#define DBG_PSST 1

#if DBG_PSST
#define _PSST_DBG(fmt, args...) {\
   printf("%d:Psst::%s():%d: " fmt, _debug_rank, \
     __FUNCTION__, __LINE__, ## args\
   );\
}
#else
#define _PSST_DBG(fmt, args...) 
#endif

class Psst : public SST::Component {
public:
  Psst(SST::ComponentId_t id, SST::Clock* clock, Params_t& params);

private:
  Psst(const Psst &c);
  bool clock(SST::Cycle_t current, SST::Time_t epoch);
  bool processEvent(SST::Time_t time, SST::Event* event);

  SST::ClockHandler_t *clockHandler;
  SST::Event::Handler_t *eventHandler;

  Params_t params;
  SST::Link *inLink;

  // Not using Params_t; models should not need component.h
  std::vector<std::pair <std::string, std::string> > model_params;

  // Interface with model.so
  void *model_lib;
  int model_lib_id;
  void (*setpsst_fp)(int);
  int (*initmodel_fp)();
  int (*dispatch_fp)();
  int (*dispatch_vm_fp)();
  int (*loadmodel_fp)(const char* model);
  int (*loadvm_fp)(const char* vm, int argc, const char** argv, long freq);
  void (*tick_fp)(void);

  // TODO: Serialization (including constructors)
  BOOST_SERIALIZE {
    _AR_DBG(SST::Psst,"start\n");
    BOOST_VOID_CAST_REGISTER(Psst *, SST::Component *);
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Component);
    ar & BOOST_SERIALIZATION_NVP(inLink);
    ar & BOOST_SERIALIZATION_NVP(clockHandler);
    ar & BOOST_SERIALIZATION_NVP(eventHandler);
    _AR_DBG(Psst,"done\n");
  }

  SAVE_CONSTRUCT_DATA( Psst ) {
    _AR_DBG(SST::Psst,"\n");
    SST::ComponentId_t id = t->_id;
    SST::Clock* clock = t->_clock;
    Params_t params = t->params;
    ar << BOOST_SERIALIZATION_NVP(id);
    ar << BOOST_SERIALIZATION_NVP(clock);
    ar << BOOST_SERIALIZATION_NVP(params);
  } 

  LOAD_CONSTRUCT_DATA( Psst ) {
    _AR_DBG(Psst,"\n");
    SST::ComponentId_t id;
    SST::Clock* clock;
    Params_t params;
    ar >> BOOST_SERIALIZATION_NVP(id);
    ar >> BOOST_SERIALIZATION_NVP(clock);
    ar >> BOOST_SERIALIZATION_NVP(params);
    ::new(t)Psst(id, clock, params);
  } 
};

#endif
