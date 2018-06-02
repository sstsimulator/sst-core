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

#include "sst_config.h"
#include <Python.h>

#include <string.h>
#include <sstream>

#include <sst/core/warnmacros.h>
#include <sst/core/model/pymodel.h>
#include <sst/core/model/pymodel_comp.h>
#include <sst/core/model/pymodel_link.h>

#include <sst/core/sst_types.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/configGraph.h>

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;

extern "C" {


static int linkInit(LinkPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *name = NULL, *lat = NULL;
    if ( !PyArg_ParseTuple(args, "s|s", &name, &lat) ) return -1;

    self->name = gModel->addNamePrefix(name);
    self->no_cut = false;
    self->latency = lat ? strdup(lat) : NULL;
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
        return NULL;
    }

    PyObject *c0, *c1;
    char *port0, *port1;
    char *lat0 = NULL, *lat1 = NULL;

    LinkPy_t *link = (LinkPy_t*)self;

    if ( !PyArg_ParseTuple(t0, "O!s|s",
                &PyModel_ComponentType, &c0, &port0, &lat0) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t0, "O!s|s",
                    &PyModel_SubComponentType, &c0, &port0, &lat0) ) {
            return NULL;
        }
    }
    if ( NULL == lat0 )
        lat0 = link->latency;

    if ( !PyArg_ParseTuple(t1, "O!s|s",
                &PyModel_ComponentType, &c1, &port1, &lat1) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t1, "O!s|s",
                    &PyModel_SubComponentType, &c1, &port1, &lat1) ) {
            return NULL;
        }
    }
    if ( NULL == lat1 )
        lat1 = link->latency;

    if ( NULL == lat0 || NULL == lat1 ) {
        gModel->getOutput()->fatal(CALL_INFO, 1, "No Latency specified for link %s\n", link->name);
        return NULL;
    }

    ComponentId_t id0, id1;
    id0 = getComp(c0)->id;
    id1 = getComp(c1)->id;

	gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Connecting components %" PRIu64 " and %" PRIu64 " to Link %s (lat: %p %p)\n",
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
    {   NULL, NULL, 0, NULL }
};


PyTypeObject PyModel_LinkType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.Link",                /* tp_name */
    sizeof(LinkPy_t),          /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)linkDealloc,   /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST Link",                /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    linkMethods,               /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)linkInit,        /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
    0,                         /* tp_free */
    0,                         /* tp_is_gc */
    0,                         /* tp_bases */
    0,                         /* tp_mro */
    0,                         /* tp_cache */
    0,                         /* tp_subclasses */
    0,                         /* tp_weaklist */
    0,                         /* tp_del */
    0,                         /* tp_version_tag */
};



}  /* extern C */


