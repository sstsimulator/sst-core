// -*- c++ -*-

// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYTHON_PYMODEL_PORTMODULE_H
#define SST_CORE_MODEL_PYTHON_PYMODEL_PORTMODULE_H

#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <string>
#include <vector>

namespace SST {
class ConfigPortModule;
}

extern "C" {

struct PortModulePy_t;
struct PyPortModule;

struct PyPortModule
{
    PortModulePy_t*    pobj;
    SST::ComponentId_t id;   // ID of component this port module is loaded at
    unsigned           lkup; // Index of this port module at port
    std::string        port; // Port name

    PyPortModule(PortModulePy_t* pobj, SST::ComponentId_t id, unsigned lkup, const char* port) :
        pobj(pobj),
        id(id),
        lkup(lkup),
        port(port)
    {}
    ~PyPortModule() = default;
    int                    compare(PyPortModule* other);
    SST::ConfigPortModule* getPortModule();

    PyPortModule(const PyPortModule&)            = delete;
    PyPortModule& operator=(const PyPortModule&) = delete;
};

struct PortModulePy_t
{
    PyObject_HEAD PyPortModule* obj;
};

extern PyTypeObject PyModel_PortModuleType;

static inline SST::ConfigPortModule*
getPortModule(PyObject* pobj)
{
    SST::ConfigPortModule* pm = ((PortModulePy_t*)pobj)->obj->getPortModule();
    if ( pm == nullptr ) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to find ConfigPortModule");
    }
    return pm;
}

} /* extern C */

#endif // SST_CORE_MODEL_PYTHON_PYMODEL_PORTMODULE_H
