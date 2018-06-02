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

#include "sst/core/model/element_python.h"

#include "sst/core/output.h"
#include "sst/core/simulation.h"

namespace SST {

SSTElementPythonModule::SSTElementPythonModule(std::string library) :
    library(library),
    primary_module(NULL)
{
    pylibrary = "py" + library;
    sstlibrary = "sst." + library;
}

void
SSTElementPythonModule::addPrimaryModule(char* file)
{
    if ( primary_module == NULL ) {
        primary_module = file;
    }
    else {
        Simulation::getSimulationOutput().fatal(CALL_INFO,1,"SSTElementPythonModule::addPrimaryModule: Attempt to add second primary module.\n");
    }
}

void
SSTElementPythonModule::addSubModule(std::string name, char* file)
{
    sub_modules.push_back(std::make_pair(name,file));
}

void*
SSTElementPythonModule::load()
{
    if ( primary_module == NULL ) {
        Simulation::getSimulationOutput().fatal(CALL_INFO,1,"SSTElementPythonModule: Primary module not set.  Use addPrimaryModule().\n");
    }
    PyObject *code = Py_CompileString(primary_module, pylibrary.c_str(), Py_file_input);
    PyObject *module = PyImport_ExecCodeModule(const_cast<char*>(sstlibrary.c_str()), code);
    
    for ( auto item : sub_modules ) {
        std::string pylib = pylibrary + "-" + item.first;
        std::string sstlib = sstlibrary + "." + item.first;
        PyObject* subcode = Py_CompileString(item.second, pylib.c_str(), Py_file_input);
        PyObject* submodule = PyImport_ExecCodeModule(const_cast<char*>(sstlib.c_str()), subcode);
        PyModule_AddObject(module, item.first.c_str(), submodule);
    }
    
    return module;
}

} // namespace SST

