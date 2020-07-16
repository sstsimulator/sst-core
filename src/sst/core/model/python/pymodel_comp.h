// -*- c++ -*-

// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYMODEL_COMP_H
#define SST_CORE_MODEL_PYMODEL_COMP_H

#include "sst/core/sst_types.h"

extern "C" {


struct ComponentPy_t;
struct PyComponent;

struct ComponentHolder {
    ComponentPy_t *pobj;
    // ConfigComponent* config;
    ComponentId_t id;
    
    ComponentHolder(ComponentPy_t *pobj, ComponentId_t id) : pobj(pobj), id(id) { }
    virtual ~ComponentHolder() { }
    virtual ConfigComponent* getComp();
    virtual int compare(ComponentHolder *other);
    virtual std::string getName();
    ComponentId_t getID();
    ConfigComponent* getSubComp(const std::string& name, int slot_num);
};

struct PyComponent : ComponentHolder {
    uint16_t subCompId;

    PyComponent(ComponentPy_t *pobj, ComponentId_t id) : ComponentHolder(pobj,id), subCompId(0) { }
    ~PyComponent() {}
};

struct PySubComponent : ComponentHolder {
    PySubComponent(ComponentPy_t *pobj, ComponentId_t id) : ComponentHolder(pobj,id) { }
    ~PySubComponent() {}
    int getSlot();
};


struct ComponentPy_t {
    PyObject_HEAD
    ComponentHolder *obj;
};

extern PyTypeObject PyModel_ComponentType;
extern PyTypeObject PyModel_SubComponentType;

static inline ConfigComponent* getComp(PyObject *pobj) {
    ConfigComponent *c = ((ComponentPy_t*)pobj)->obj->getComp();
    if ( c == nullptr ) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to find ConfigComponent");
    }
    return c;
}


}  /* extern C */


#endif
