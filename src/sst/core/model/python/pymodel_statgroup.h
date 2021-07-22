// -*- c++ -*-

// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYTHON_PYMODEL_STATGROUP_H
#define SST_CORE_MODEL_PYTHON_PYMODEL_STATGROUP_H

#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

namespace SST {
class ConfigStatGroup;
class ConfigStatOutput;

extern "C" {

struct StatGroupPy_t
{
    PyObject_HEAD SST::ConfigStatGroup* ptr;
};

struct StatOutputPy_t
{
    PyObject_HEAD size_t id; /* Index into Graph's statOutputs array */
    ConfigStatOutput*    ptr;
};

extern PyTypeObject PyModel_StatGroupType;
extern PyTypeObject PyModel_StatOutputType;

} /* extern C */

} // namespace SST

#endif // SST_CORE_MODEL_PYTHON_PYMODEL_STATGROUP_H
