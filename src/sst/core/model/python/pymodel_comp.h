// -*- c++ -*-

// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYTHON_PYMODEL_COMP_H
#define SST_CORE_MODEL_PYTHON_PYMODEL_COMP_H

#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <string>

namespace SST {
class ConfigComponent;
} // namespace SST

extern "C" {

struct ComponentPy_t;
struct PyComponent;

struct ComponentHolder
{
    ComponentPy_t*     pobj;
    SST::ComponentId_t id;

    ComponentHolder(ComponentPy_t* pobj, SST::ComponentId_t id) : pobj(pobj), id(id) {}
    virtual ~ComponentHolder() {}
    virtual SST::ConfigComponent* getComp();
    virtual int                   compare(ComponentHolder* other);
    virtual std::string           getName();
    SST::ComponentId_t            getID();
    SST::ConfigComponent*         getSubComp(const std::string& name, int slot_num);
};

struct PyComponent : ComponentHolder
{
    uint16_t subCompId;

    PyComponent(ComponentPy_t* pobj, SST::ComponentId_t id) : ComponentHolder(pobj, id), subCompId(0) {}
    ~PyComponent() {}
};

struct PySubComponent : ComponentHolder
{
    PySubComponent(ComponentPy_t* pobj, SST::ComponentId_t id) : ComponentHolder(pobj, id) {}
    ~PySubComponent() {}
    int getSlot();
};

struct ComponentPy_t
{
    PyObject_HEAD ComponentHolder* obj;
};

extern PyTypeObject PyModel_ComponentType;
extern PyTypeObject PyModel_SubComponentType;

static inline SST::ConfigComponent*
getComp(PyObject* pobj)
{
    SST::ConfigComponent* c = ((ComponentPy_t*)pobj)->obj->getComp();
    if ( c == nullptr ) { PyErr_SetString(PyExc_RuntimeError, "Failed to find ConfigComponent"); }
    return c;
}

} /* extern C */

#endif // SST_CORE_MODEL_PYTHON_PYMODEL_COMP_H
