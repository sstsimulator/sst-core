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

#include "sst_config.h"

#include "sst/core/model/python/pymodel_link.h"

#include "sst/core/component.h"
#include "sst/core/model/configGraph.h"
#include "sst/core/model/python/pymacros.h"
#include "sst/core/model/python/pymodel.h"
#include "sst/core/model/python/pymodel_comp.h"
#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <cstring>

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition* gModel;

extern "C" {

static int
linkInit(LinkPy_t* self, PyObject* args, PyObject* UNUSED(kwds))
{
    char*       name = nullptr;
    const char* lat  = nullptr;
    PyObject*   lobj = nullptr;
    PyObject*   lstr = nullptr;
    if ( !PyArg_ParseTuple(args, "s|O", &name, &lobj) ) return -1;

    char* full_name = gModel->addNamePrefix(name);

    if ( nullptr != lobj ) {
        lstr = PyObject_Str(lobj);
        lat  = SST_ConvertToCppString(lstr);
    }

    self->link_id = gModel->createLink(full_name, lat);

    free(full_name);
    Py_XDECREF(lstr);

    return 0;
}

static void
linkDealloc(LinkPy_t* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
linkConnect(PyObject* self, PyObject* args)
{
    PyObject *t0, *t1;
    if ( !PyArg_ParseTuple(args, "O!O!", &PyTuple_Type, &t0, &PyTuple_Type, &t1) ) {
        return nullptr;
    }

    PyObject *  c0, *c1;
    char *      port0, *port1;
    PyObject *  l0 = nullptr, *l1 = nullptr, *lstr0 = nullptr, *lstr1 = nullptr;
    const char *lat0 = nullptr, *lat1 = nullptr;

    LinkPy_t* link = (LinkPy_t*)self;

    if ( !PyArg_ParseTuple(t0, "O!s|O", &PyModel_ComponentType, &c0, &port0, &l0) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t0, "O!s|O", &PyModel_SubComponentType, &c0, &port0, &l0) ) {
            return nullptr;
        }
    }

    if ( nullptr != l0 ) {
        lstr0 = PyObject_Str(l0);
        lat0  = SST_ConvertToCppString(lstr0);
    }


    if ( !PyArg_ParseTuple(t1, "O!s|O", &PyModel_ComponentType, &c1, &port1, &l1) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t1, "O!s|O", &PyModel_SubComponentType, &c1, &port1, &l1) ) {
            return nullptr;
        }
    }

    if ( nullptr != l1 ) {
        lstr1 = PyObject_Str(l1);
        lat1  = SST_ConvertToCppString(lstr1);
    }

    ComponentId_t id0, id1;
    id0 = getComp(c0)->id;
    id1 = getComp(c1)->id;

    gModel->addLink(id0, link->link_id, port0, lat0);
    gModel->addLink(id1, link->link_id, port1, lat1);

    Py_XDECREF(lstr0);
    Py_XDECREF(lstr1);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
linkConnectNonLocal(PyObject* self, PyObject* args)
{
    PyObject *t0, *t1;
    if ( !PyArg_ParseTuple(args, "O!O!", &PyTuple_Type, &t0, &PyTuple_Type, &t1) ) {
        return nullptr;
    }

    PyObject*   c0;
    char*       port0;
    PyObject*   l0     = nullptr;
    PyObject*   lstr0  = nullptr;
    const char* lat0   = nullptr;
    int         rank   = 0;
    int         thread = 0;

    LinkPy_t* link = (LinkPy_t*)self;

    if ( !PyArg_ParseTuple(t0, "O!s|O", &PyModel_ComponentType, &c0, &port0, &l0) ) {
        PyErr_Clear();
        if ( !PyArg_ParseTuple(t0, "O!s|O", &PyModel_SubComponentType, &c0, &port0, &l0) ) {
            return nullptr;
        }
    }

    if ( nullptr != l0 ) {
        lstr0 = PyObject_Str(l0);
        lat0  = SST_ConvertToCppString(lstr0);
    }

    PyArg_ParseTuple(t1, "ii", &rank, &thread);

    ComponentId_t id0;
    id0 = getComp(c0)->id;

    gModel->addLink(id0, link->link_id, port0, lat0);
    gModel->addNonLocalLink(link->link_id, rank, thread);

    Py_XDECREF(lstr0);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
linkSetNonLocal(PyObject* self, PyObject* args)
{
    int rank   = 0;
    int thread = 0;
    if ( !PyArg_ParseTuple(args, "ii", &rank, &thread) ) {
        return nullptr;
    }

    LinkPy_t* link = (LinkPy_t*)self;

    gModel->addNonLocalLink(link->link_id, rank, thread);

    return SST_ConvertToPythonLong(0);
}

static PyObject*
linkSetNoCut(PyObject* self, PyObject* UNUSED(args))
{
    LinkPy_t* link = (LinkPy_t*)self;
    gModel->setLinkNoCut(link->link_id);
    return PyBool_FromLong(0);
}


static PyMethodDef linkMethods[] = { { "connect", linkConnect, METH_VARARGS, "Connects two components to a Link" },
    { "connectNonLocal", linkConnectNonLocal, METH_VARARGS,
        "Connects one component to a Link and annotates it as nonlocal using the provided rank and thread" },
    { "setNonLocal", linkSetNonLocal, METH_VARARGS, "Annotates link as nonlocal using the provided rank and thread" },
    { "setNoCut", linkSetNoCut, METH_NOARGS, "Specifies that this link should not be partitioned across" },
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
    0,                          /* tp_vectorcall_offset */
    nullptr,                    /* tp_getattr */
    nullptr,                    /* tp_setattr */
    nullptr,                    /* tp_as_sync */
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
    nullptr,                    /* tp_finalize */
    SST_TP_VECTORCALL           /* Python3.8+ */
    SST_TP_PRINT_DEP            /* Python3.8 only */
    SST_TP_WATCHED              /* Python3.12+ */
    SST_TP_VERSIONS_USED        /* Python3.13+ only */
};
#if PY_MAJOR_VERSION == 3
#if PY_MINOR_VERSION == 8
REENABLE_WARNING
#endif
#endif

} /* extern C */
