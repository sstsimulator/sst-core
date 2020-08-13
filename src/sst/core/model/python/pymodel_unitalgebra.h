// -*- c++ -*-

// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYMODEL_UNITALGEBRA_H
#define SST_CORE_MODEL_PYMODEL_UNITALGEBRA_H

#include <sst/core/sst_types.h>

extern "C" {


struct UnitAlgebraPy_t {
    PyObject_HEAD
    UnitAlgebra obj;
};


extern PyTypeObject PyModel_UnitAlgebraType;


}  /* extern C */


#endif
