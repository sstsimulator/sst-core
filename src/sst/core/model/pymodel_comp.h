// -*- c++ -*-

// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
    ConfigComponent* getSubComp(const std::string &name);
};

struct PyComponent : ComponentHolder {
    ComponentId_t id;
    uint16_t subCompId;

    PyComponent(ComponentPy_t *pobj) : ComponentHolder(pobj), subCompId(0) { }
    ~PyComponent() {}
    const char* getName() const ;
    ConfigComponent* getComp();
    PyComponent* getBaseObj();
    int compare(ComponentHolder *other);
};

struct PySubComponent : ComponentHolder {
    ComponentHolder *parent;

    PySubComponent(ComponentPy_t *pobj) : ComponentHolder(pobj) { }
    ~PySubComponent() {}
    const char* getName() const ;
    ConfigComponent* getComp();
    PyComponent* getBaseObj();
    int compare(ComponentHolder *other);
};


struct ComponentPy_t {
    PyObject_HEAD
    ComponentHolder *obj;
};

extern PyTypeObject PyModel_ComponentType;
extern PyTypeObject PyModel_SubComponentType;


}  /* extern C */


#endif
