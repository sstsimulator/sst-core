// -*- c++ -*-

// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYTHON_PYMODEL_STAT_H
#define SST_CORE_MODEL_PYTHON_PYMODEL_STAT_H

#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

namespace SST {
class ConfigStatistic;

extern "C" {

struct StatisticPy_t;
struct PyStatistic;

struct PyStatistic
{
    StatisticId_t id;

    PyStatistic(StatisticId_t id) : id(id) {}
    virtual ~PyStatistic() {}
    ConfigStatistic* getStat();
    int              compare(PyStatistic* other);
    StatisticId_t    getID();
};

struct StatisticPy_t
{
    PyObject_HEAD PyStatistic* obj;
};

extern PyTypeObject PyModel_StatType;

static inline ConfigStatistic*
getStat(PyObject* pobj)
{
    ConfigStatistic* c = ((StatisticPy_t*)pobj)->obj->getStat();
    if ( c == nullptr ) { PyErr_SetString(PyExc_RuntimeError, "Failed to find ConfigStatistic"); }
    return c;
}

} /* extern C */

} // namespace SST

#endif // SST_CORE_MODEL_PYTHON_PYMODEL_STAT_H
