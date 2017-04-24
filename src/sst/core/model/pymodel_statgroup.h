// -*- c++ -*-

// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_PYMODEL_STATGROUP_H
#define SST_CORE_MODEL_PYMODEL_STATGROUP_H

#include <sst/core/sst_types.h>

namespace SST {
    class ConfigStatGroup;
}

extern "C" {


struct StatGroupPy_t {
    PyObject_HEAD
    SST::ConfigStatGroup *ptr;
};


struct StatOutputPy_t {
    PyObject_HEAD
    size_t id; /* Index into Graph's statOutputs array */
    SST::ConfigStatOutput *ptr;
};

extern PyTypeObject PyModel_StatGroupType;
extern PyTypeObject PyModel_StatOutputType;


}  /* extern C */


#endif
