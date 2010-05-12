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

/*----------------------------------------------------------+----------------\
| Psst                                                      | Chad D. Kersey |
+-----------------------------------------------------------+----------------+
| Processor component for SST.                                               |
\---------------------------------------------------------------------------*/
#include <sst/memEvent.h>

#include <cstdlib>
#include <dlfcn.h>

#include "psst.h"
#include "queue.h"

#include <string>
#include <vector>
#include <utility>

using std::string; using std::vector; using std::pair;
using namespace SST;

using Models::Model;

Psst::Psst(ComponentId_t id, Clock* clock, Params_t& params) :
  Component(id, clock),
  params(params)
{ 
  string vm_path,        // Path to VM shared object.
    hd_path,             // Path to hard disk image.
    bios_path,           // Path to VM BIOS directory.
    serial_path;         // Path to character device to redirect serial to.
  vector<string> models; // Paths to ".so"s for models

  double f;              // Clock frequency.

  // Read VM/model paramters from configuration file.
  Params_t::iterator it = params.begin();
  while( it != params.end() ) {
    if (!it->first.compare("clock")) sscanf( it->second.c_str(), "%lf", &f);
    if (!it->first.compare("vm")) vm_path = it->second;
    if (!it->first.compare("hd")) hd_path = it->second;
    if (!it->first.compare("bios")) bios_path = it->second;
    if (!it->first.compare("serial")) serial_path = it->second;
    if (!it->first.compare(0, 5, "model")) models.push_back(it->second);
    if (!it->first.compare(0, 2, "__")) model_params.push_back(*it);
    it++;
  }

  // Init clock tick handler
  clockHandler= new EventHandler<Psst, bool, Cycle_t, Time_t>
      (this, &Psst::clock);
  ClockRegister( f, clockHandler );

  // Init link and a handler
  eventHandler = new EventHandler <Psst, bool, Time_t, Event* >
    (this, &Psst::processEvent );
  inLink = LinkAdd( "input", eventHandler );

  // Load copy of "model.so", needed by all VMs and models.
  if ((model_lib = dlopen("model.so", RTLD_LAZY|RTLD_GLOBAL)) == NULL) {
    _PSST_DBG("Could not find \"%s\".\n", "model.so");
    exit(1);
  }

  // Get a pointer to the "init model.so context" function, needed because 
  // this single .so can be shared by several instances of Psst.
  if ( (initmodel_fp = (int (*)())dlsym(model_lib, "initModelSo")) == NULL) {
    exit(1);
  }

  // Then initialize this instance of Psst within model.so
  model_lib_id = initmodel_fp();

  // This must be called with the return value of initmodel_fp() before any
  // feature from model.so is used.
  if ( (setpsst_fp = (void (*)(int))dlsym(model_lib, "setPsst")) == NULL) {
    exit(1);
  }

  // Set model_params within model.so context to reflect read model parameters
  typedef vector<pair <string, string> > param_t;
  param_t ***mp_p;
  if ((mp_p = (param_t ***)dlsym(model_lib, "model_params")) == NULL) {
    exit(1);
  }
  **mp_p = &model_params;

  // Get functions from model.so
  typedef int (*dispf_t)(void);
  typedef int (*loadmf_t)(const char *);
  typedef int (*loadvmf_t)(const char*, const int, const char**, long);
  typedef void (*tickf_t)(void);
  if ((dispatch_fp = (dispf_t)dlsym(model_lib, "dispatch")) == NULL) {
    exit(1);
  }
  
  if ((dispatch_vm_fp=(dispf_t)dlsym(model_lib,"dispatch_to_vm")) == NULL) {
    exit(1);
  }

  if ((loadmodel_fp = (loadmf_t)dlsym(model_lib, "load_model")) == NULL) {
    exit(1);
  }
  
  if ((loadvm_fp = (loadvmf_t)dlsym(model_lib, "load_vm")) == NULL) {
    exit(1);
  }

  if ((tick_fp = (tickf_t)dlsym(model_lib, "tick_vms")) == NULL) {
    exit(1);
  }

  // Load VM, initialize.
  typedef int (*main_fp)(int, const char**);

  // TODO: Genericise the VM arguments.
  const char *vm_args[] = {
    "qemu",                          // Let the VM think it was run as "qemu"
    "-hda", hd_path.c_str(),
    "-boot", "c",              
    "-L", bios_path.c_str(),
    "-serial", serial_path.c_str(),  // Fake a serial port on the given device
    "-nographic",                    // Don't use VGA
    "-monitor", "/dev/null", 
    "-icount", "auto",               // Enable instruction counter (important)
    "-no-hpet"                       // The HPET breaks, disable it.
  };

  if (!loadvm_fp(vm_path.c_str(), 15, vm_args, f)) { 
    _PSST_DBG("loadvm_fp failed!"); 
    exit(1); 
  }

  // Load models.
  for (vector<string>::iterator i = models.begin(); i != models.end(); i++) {
    loadmodel_fp(i->c_str());
  }
}

bool Psst::clock( Cycle_t current, Time_t epoch ) {
  // Advance the VM (This may move to processEvent())
  // TODO: if next tick is before current cycle, don't tick.

  //Make sure we're talking to the right model.so instance
  setpsst_fp(model_lib_id);

  tick_fp(); /*Run until next callback.*/
  while (dispatch_fp()); /*Dispatch all VM-generated events.*/
  while (dispatch_vm_fp()); /*Dispatch all model-generated events.*/
  
  return false;
}

bool Psst::processEvent( Time_t time, Event* event ) {
  _PSST_DBG( "Got event, type = \"%s\".\n", typeid(*event).name() );
  // Got it, but I don't have anything to do with it (yet)
  return false;
}

/* Factory function for SST module interface. */
extern "C" {
  Psst* PsstAllocComponent(SST::ComponentId_t id,  SST::Clock* clock,
    SST::Component::Params_t& params) {
    return new Psst(id, clock, params);
  }
}

BOOST_CLASS_EXPORT(Psst)

BOOST_CLASS_EXPORT_TEMPLATE4(SST::EventHandler,
    Psst, bool, SST::Time_t, SST::Event *)
