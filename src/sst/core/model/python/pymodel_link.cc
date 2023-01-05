// -*- c++ -*-

// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/model/python/pymodel_link.h"

#include "sst/core/component.h"
#include "sst/core/configGraph.h"
#include "sst/core/model/python/pymacros.h"
#include "sst/core/model/python/pymodel.h"
#include "sst/core/model/python/pymodel_comp.h"
#include "sst/core/simulation.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <string.h>

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition* gModel;

extern "C" {

static int
linkInit(LinkPy_t* self, PyObject* args, PyObject* UNUSED(kwds))
{
    char*       name = nullptr;
    const char* lat  = nullptr;
    PyObject *  lobj = nullptr, *lstr = nullptr;
    if ( !PyArg_ParseTuple(args, "s|O", &name, &lobj) ) return -1;

    self->name   = gModel->addNamePrefix(name);
    self->no_cut = false;

    if ( nullptr != lobj ) {
        lstr = PyObject_CallMethod(lobj, (char*)"__str__", nullptr);
        lat  = SST_ConvertToCppString(lstr);
    }
    self->latency = lat ? strdup(lat) : nullptr;

    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating Link %s\n", self->name);

    return 0;
}

static void
linkDealloc(LinkPy_t* self)
{
    if ( self->name ) free(self->name);
    if ( self->latency ) free(self->latency);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
linkConnect(PyObject* self, PyObject* args)
{
    PyObject *t0, *t1;
    if ( !PyArg_ParseTuple(args, "O!O!", &PyTuple_Type, &t0, &PyTuple_Type, &t1) ) { return nullptr; }

    PyObject *  c0, *c1;
    char *      port0, *port1;
    PyObject *  l0 = nullptr, *l1 = nullptr, *lstr0 = nullptr, *lstr1 = nullptr;
    const char *lat0 = nullptr, *lat1 = nullptr;

    LinkPy_t* link = (LinkPy_t*)self;

    if ( !PyArg_ParseTuple(t0, "O!s|O", &PyModel_ComponentType, &c0, &port0, &l0) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t0, "O!s|O", &PyModel_SubComponentType, &c0, &port0, &l0) ) { return nullptr; }
    }

    if ( nullptr != l0 ) {
        lstr0 = PyObject_CallMethod(l0, (char*)"__str__", nullptr);
        lat0  = SST_ConvertToCppString(lstr0);
    }

    if ( nullptr == lat0 ) lat0 = link->latency;


    if ( !PyArg_ParseTuple(t1, "O!s|O", &PyModel_ComponentType, &c1, &port1, &l1) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t1, "O!s|O", &PyModel_SubComponentType, &c1, &port1, &l1) ) { return nullptr; }
    }

    if ( nullptr != l1 ) {
        lstr1 = PyObject_CallMethod(l1, (char*)"__str__", nullptr);
        lat1  = SST_ConvertToCppString(lstr1);
    }

    if ( nullptr == lat1 ) lat1 = link->latency;

    if ( nullptr == lat0 || nullptr == lat1 ) {
        gModel->getOutput()->fatal(CALL_INFO, 1, "No Latency specified for link %s\n", link->name);
        return nullptr;
    }

    ComponentId_t id0, id1;
    id0 = getComp(c0)->id;
    id1 = getComp(c1)->id;

    gModel->getOutput()->verbose(
        CALL_INFO, 3, 0, "Connecting components %" PRIu64 " and %" PRIu64 " to Link %s (lat: %s %s)\n", id0, id1,
        ((LinkPy_t*)self)->name, lat0, lat1);
    gModel->addLink(id0, link->name, port0, lat0, link->no_cut);
    gModel->addLink(id1, link->name, port1, lat1, link->no_cut);

    Py_XDECREF(lstr0);
    Py_XDECREF(lstr1);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
linkSetNoCut(PyObject* self, PyObject* UNUSED(args))
{
    LinkPy_t* link = (LinkPy_t*)self;
    bool      prev = link->no_cut;
    link->no_cut   = true;
    gModel->setLinkNoCut(link->name);
    return PyBool_FromLong(prev ? 1 : 0);
}

static PyMethodDef linkMethods[] = { { "connect", linkConnect, METH_VARARGS, "Connects two components to a Link" },
                                     { "setNoCut", linkSetNoCut, METH_NOARGS,
                                       "Specifies that this link should not be partitioned across" },
                                     { nullptr, nullptr, 0, nullptr } };

#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
DISABLE_WARN_DEPRECATED_DECLARATION
#endif
#endif
PyTypeObject PyModel_LinkType = {
    SST_PY_OBJ_HEAD "sst.Link", /* tp_name */
    sizeof(LinkPy_t),           /* tp_basicsize */
    0,                          /* tp_itemsize */
    (destructor)linkDealloc,    /* tp_dealloc */
    SST_TP_VECTORCALL_OFFSET    /* Python3 only */
        SST_TP_PRINT            /* Python2 only */
    nullptr,                    /* tp_getattr */
    nullptr,                    /* tp_setattr */
    SST_TP_COMPARE(nullptr)     /* Python2 only */
    SST_TP_AS_SYNC              /* Python3 only */
    nullptr,                    /* tp_repr */
    nullptr,                    /* tp_as_number */
    nullptr,                    /* tp_as_sequence */
    nullptr,                    /* tp_as_mapping */
    nullptr,                    /* tp_hash */
    nullptr,                    /* tp_call */
    nullptr,                    /* tp_str */
    nullptr,                    /* tp_getattro */
    nullptr,                    /* tp_setattro */
    nullptr,                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,         /* tp_flags */
    "SST Link",                 /* tp_doc */
    nullptr,                    /* tp_traverse */
    nullptr,                    /* tp_clear */
    nullptr,                    /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    nullptr,                    /* tp_iter */
    nullptr,                    /* tp_iternext */
    linkMethods,                /* tp_methods */
    nullptr,                    /* tp_members */
    nullptr,                    /* tp_getset */
    nullptr,                    /* tp_base */
    nullptr,                    /* tp_dict */
    nullptr,                    /* tp_descr_get */
    nullptr,                    /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc)linkInit,         /* tp_init */
    nullptr,                    /* tp_alloc */
    nullptr,                    /* tp_new */
    nullptr,                    /* tp_free */
    nullptr,                    /* tp_is_gc */
    nullptr,                    /* tp_bases */
    nullptr,                    /* tp_mro */
    nullptr,                    /* tp_cache */
    nullptr,                    /* tp_subclasses */
    nullptr,                    /* tp_weaklist */
    nullptr,                    /* tp_del */
    0,                          /* tp_version_tag */
    SST_TP_FINALIZE             /* Python3 only */
        SST_TP_VECTORCALL       /* Python3 only */
            SST_TP_PRINT_DEP    /* Python3.8 only */
};
#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
REENABLE_WARNING
#endif
#endif

} /* extern C */
