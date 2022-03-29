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

#ifndef SST_CORE_MODEL_PYTHON_PYMODEL_LINK_H
#define SST_CORE_MODEL_PYTHON_PYMODEL_LINK_H

#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

extern "C" {

struct LinkPy_t
{
    PyObject_HEAD char* name;
    bool                no_cut;
    char*               latency;
};

extern PyTypeObject PyModel_LinkType;

} /* extern C */

#endif // SST_CORE_MODEL_PYTHON_PYMODEL_LINK_H
