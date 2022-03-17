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

#ifndef SST_CORE_MODEL_PYTHON_PYMACROS_H
#define SST_CORE_MODEL_PYTHON_PYMACROS_H

#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#if PY_MAJOR_VERSION >= 3
#define SST_PY_OBJ_HEAD              PyVarObject_HEAD_INIT(nullptr, 0)
#define SST_ConvertToPythonLong(x)   PyLong_FromLong(x)
#define SST_ConvertToPythonBool(x)   PyBool_FromLong(x)
#define SST_ConvertToCppLong(x)      PyLong_AsLong(x)
#define SST_ConvertToPythonString(x) PyUnicode_FromString(x)
#define SST_ConvertToCppString(x)    PyUnicode_AsUTF8(x)
#define SST_TP_FINALIZE              nullptr,
#define SST_TP_VECTORCALL_OFFSET     0,
#define SST_TP_PRINT
#define SST_TP_COMPARE(x)
#define SST_TP_RICH_COMPARE(x)                    x,
#define SST_TP_AS_SYNC                            nullptr,
#define SST_PY_INIT_MODULE(name, methods, moddef) PyModule_Create(&moddef)
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

#else

#define Py_TYPE(ob)                  (((PyObject*)(ob))->ob_type)
#define SST_PY_OBJ_HEAD              PyVarObject_HEAD_INIT(nullptr, 0)
#define SST_ConvertToPythonLong(x)   PyInt_FromLong(x)
#define SST_ConvertToCppLong(x)      PyInt_AsLong(x)
#define SST_ConvertToPythonString(x) PyString_FromString(x)
#define SST_ConvertToCppString(x)    PyString_AsString(x)
#define SST_TP_FINALIZE
#define SST_TP_VECTORCALL_OFFSET
#define SST_TP_PRINT           nullptr,
#define SST_TP_COMPARE(x)      x,
#define SST_TP_RICH_COMPARE(x) nullptr,
#define SST_TP_AS_SYNC
#define SST_TP_VECTORCALL
#define SST_TP_PRINT_DEP
#define SST_PY_INIT_MODULE(name, methods, moddef) Py_InitModule(name, methods)

// Number protocol macros
#define SST_NB_DIVIDE(x)                          x,
#define SST_NB_COERCE                             nullptr,
#define SST_NB_INTLONG(x)                         x, x,
#define SST_NB_RESERVED
#define SST_NB_OCT               nullptr,
#define SST_NB_HEX               nullptr,
#define SST_NB_INPLACE_DIVIDE(x) x,
#define SST_NB_MATRIX_MULTIPLY
#define SST_NB_INPLACE_MATRIX_MULTIPLY
#endif

#endif // SST_CORE_MODEL_PYTHON_PYMACROS_H
