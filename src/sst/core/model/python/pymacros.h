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

#ifndef SST_CORE_MODEL_PYTHON_PYMACROS_H
#define SST_CORE_MODEL_PYTHON_PYMACROS_H

#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#define SST_PY_OBJ_HEAD              PyVarObject_HEAD_INIT(nullptr, 0)
#define SST_ConvertToPythonLong(x)   PyLong_FromLong(x)
#define SST_ConvertToPythonBool(x)   PyBool_FromLong(x)
#define SST_ConvertToCppLong(x)      PyLong_AsLong(x)
#define SST_ConvertToPythonString(x) PyUnicode_FromString(x)
#define SST_ConvertToCppString(x)    PyUnicode_AsUTF8(x)

#if PY_MINOR_VERSION == 8
#define SST_TP_PRINT_DEP nullptr,
//#define SST_TP_PRINT_DEP DISABLE_WARN_DEPRECATED_DECLARATION nullptr, REENABLE_WARNING
#else
#define SST_TP_PRINT_DEP
#endif

#if PY_MINOR_VERSION >= 8
#define SST_TP_VECTORCALL nullptr,
#else
#define SST_TP_VECTORCALL
#endif

#if PY_MINOR_VERSION >= 12
#define SST_TP_WATCHED 0,
#else
#define SST_TP_WATCHED
#endif

// Number protocol macros
#define SST_NB_DIVIDE(x)
#define SST_NB_COERCE
#define SST_NB_INTLONG(x) x,
#define SST_NB_RESERVED   nullptr,
#define SST_NB_OCT
#define SST_NB_HEX
#define SST_NB_INPLACE_DIVIDE(x)
#define SST_NB_MATRIX_MULTIPLY         nullptr,
#define SST_NB_INPLACE_MATRIX_MULTIPLY nullptr,

#endif // SST_CORE_MODEL_PYTHON_PYMACROS_H
