// -*- c++ -*-

// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <string.h>

#include "sst/core/model/pymodel.h"
#include "sst/core/model/pymodel_comp.h"
#include "sst/core/model/pymodel_link.h"

#include "sst/core/sst_types.h"
#include "sst/core/simulation.h"
#include "sst/core/component.h"
#include "sst/core/configGraph.h"

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;

extern "C" {


static int linkInit(LinkPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *name = nullptr, *lat = nullptr;
    if ( !PyArg_ParseTuple(args, "s|s", &name, &lat) ) return -1;

    self->name = gModel->addNamePrefix(name);
    self->no_cut = false;
    self->latency = lat ? strdup(lat) : nullptr;
    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating Link %s\n", self->name);

    return 0;
}

static void linkDealloc(LinkPy_t *self)
{
    if ( self->name ) free(self->name);
    if ( self->latency ) free(self->latency);
    self->ob_type->tp_free((PyObject*)self);
}


static PyObject* linkConnect(PyObject* self, PyObject *args)
{
    PyObject *t0, *t1;
    if ( !PyArg_ParseTuple(args, "O!O!",
                &PyTuple_Type, &t0,
                &PyTuple_Type, &t1) ) {
        return nullptr;
    }

    PyObject *c0, *c1;
    char *port0, *port1;
    char *lat0 = nullptr, *lat1 = nullptr;

    LinkPy_t *link = (LinkPy_t*)self;

    if ( !PyArg_ParseTuple(t0, "O!s|s",
                &PyModel_ComponentType, &c0, &port0, &lat0) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t0, "O!s|s",
                    &PyModel_SubComponentType, &c0, &port0, &lat0) ) {
            return nullptr;
        }
    }
    if ( nullptr == lat0 )
        lat0 = link->latency;

    if ( !PyArg_ParseTuple(t1, "O!s|s",
                &PyModel_ComponentType, &c1, &port1, &lat1) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t1, "O!s|s",
                    &PyModel_SubComponentType, &c1, &port1, &lat1) ) {
            return nullptr;
        }
    }
    if ( nullptr == lat1 )
        lat1 = link->latency;

    if ( nullptr == lat0 || nullptr == lat1 ) {
        gModel->getOutput()->fatal(CALL_INFO, 1, "No Latency specified for link %s\n", link->name);
        return nullptr;
    }

    ComponentId_t id0, id1;
    id0 = getComp(c0)->id;
    id1 = getComp(c1)->id;

    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Connecting components %" PRIu64 " and %" PRIu64 " to Link %s (lat: %s %s)\n",
            id0, id1, ((LinkPy_t*)self)->name, lat0, lat1);
    gModel->addLink(id0, link->name, port0, lat0, link->no_cut);
    gModel->addLink(id1, link->name, port1, lat1, link->no_cut);


    return PyInt_FromLong(0);
}


static PyObject* linkSetNoCut(PyObject* self, PyObject *UNUSED(args))
{
    LinkPy_t *link = (LinkPy_t*)self;
    bool prev = link->no_cut;
    link->no_cut = true;
    gModel->setLinkNoCut(link->name);
    return PyBool_FromLong(prev ? 1 : 0);
}




static PyMethodDef linkMethods[] = {
    {   "connect",
        linkConnect, METH_VARARGS,
        "Connects two components to a Link"},
    {   "setNoCut",
        linkSetNoCut, METH_NOARGS,
        "Specifies that this link should not be partitioned across"},
    {   nullptr, nullptr, 0, nullptr }
};


PyTypeObject PyModel_LinkType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "sst.Link",                /* tp_name */
    sizeof(LinkPy_t),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)linkDealloc,   /* tp_dealloc */
    nullptr,                   /* tp_print */
    nullptr,                   /* tp_getattr */
    nullptr,                   /* tp_setattr */
    nullptr,                   /* tp_compare */
    nullptr,                   /* tp_repr */
    nullptr,                   /* tp_as_number */
    nullptr,                   /* tp_as_sequence */
    nullptr,                   /* tp_as_mapping */
    nullptr,                   /* tp_hash */
    nullptr,                   /* tp_call */
    nullptr,                   /* tp_str */
    nullptr,                   /* tp_getattro */
    nullptr,                   /* tp_setattro */
    nullptr,                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST Link",                /* tp_doc */
    nullptr,                   /* tp_traverse */
    nullptr,                   /* tp_clear */
    nullptr,                   /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    nullptr,                   /* tp_iter */
    nullptr,                   /* tp_iternext */
    linkMethods,               /* tp_methods */
    nullptr,                   /* tp_members */
    nullptr,                   /* tp_getset */
    nullptr,                   /* tp_base */
    nullptr,                   /* tp_dict */
    nullptr,                   /* tp_descr_get */
    nullptr,                   /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)linkInit,        /* tp_init */
    nullptr,                   /* tp_alloc */
    nullptr,                   /* tp_new */
    nullptr,                   /* tp_free */
    nullptr,                   /* tp_is_gc */
    nullptr,                   /* tp_bases */
    nullptr,                   /* tp_mro */
    nullptr,                   /* tp_cache */
    nullptr,                   /* tp_subclasses */
    nullptr,                   /* tp_weaklist */
    nullptr,                   /* tp_del */
    0,                         /* tp_version_tag */
};



}  /* extern C */


