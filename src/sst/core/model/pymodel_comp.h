// -*- c++ -*-

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYMODEL_COMP_H
#define SST_CORE_MODEL_PYMODEL_COMP_H

#include <sst/core/sst_types.h>

extern "C" {


struct ComponentPy_t;
struct PyComponent;

struct ComponentHolder {
    ComponentPy_t *pobj;
	char *name;
    ComponentHolder(ComponentPy_t *pobj) : pobj(pobj), name(NULL) { }
    virtual ~ComponentHolder() { free(name); }
    virtual ConfigComponent* getComp() = 0;
    virtual PyComponent* getBaseObj() = 0;
    virtual int compare(ComponentHolder *other) = 0;
    virtual const char* getName() const = 0;
    ComponentId_t getID();
    ConfigComponent* getSubComp(const std::string &name, int slot_num);
};

struct PyComponent : ComponentHolder {
    ComponentId_t id;
    uint16_t subCompId;

    PyComponent(ComponentPy_t *pobj) : ComponentHolder(pobj), subCompId(0) { }
    ~PyComponent() {}
    const char* getName() const override;
    ConfigComponent* getComp() override;
    PyComponent* getBaseObj() override;
    int compare(ComponentHolder *other) override;
};

struct PySubComponent : ComponentHolder {
    ComponentHolder *parent;

    int slot;
    
    PySubComponent(ComponentPy_t *pobj) : ComponentHolder(pobj) { }
    ~PySubComponent() {}
    const char* getName() const override;
    ConfigComponent* getComp() override;
    PyComponent* getBaseObj() override;
    int compare(ComponentHolder *other) override;
    int getSlot() const;
};


struct ComponentPy_t {
    PyObject_HEAD
    ComponentHolder *obj;
};

extern PyTypeObject PyModel_ComponentType;
extern PyTypeObject PyModel_SubComponentType;

static inline ConfigComponent* getComp(PyObject *pobj) {
    ConfigComponent *c = ((ComponentPy_t*)pobj)->obj->getComp();
    if ( c == NULL ) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to find ConfigComponent");
    }
    return c;
}


}  /* extern C */


#endif
