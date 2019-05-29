// -*- c++ -*-

// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

// Python header files
#include <sst/core/warnmacros.h>

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING
#include <traceback.h>
#include <frameobject.h>

#include "sst/core/model/element_python.h"

#include "sst/core/output.h"
#include "sst/core/simulation.h"

namespace SST {

SST_ELI_DEFINE_CTOR_EXTERN(SSTElementPythonModule)
SST_ELI_DEFINE_INFO_EXTERN(SSTElementPythonModule)

// Utility function to parse the python exceptions from loading
// modules and format and print them on abort.
void abortOnPyErr(uint32_t line, const char* file, const char* func,
                  uint32_t exit_code, const char* format, ...) {
    PyObject *exc, *val, *tb;
    PyErr_Fetch(&exc,&val,&tb);
    PyErr_NormalizeException(&exc,&val,&tb);

    // Get the exception name
    char* exc_name = PyExceptionClass_Name(exc);
    if (exc_name != NULL) {
        char *dot = strrchr(exc_name, '.');
        if (dot != NULL)
            exc_name = dot+1;
    }

    // Create error message
    char buffer[2000];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 2000, format, args);
    //perror (buffer);
    va_end (args);
    
    
    std::stringstream stream;
    stream << buffer;
    
    // Need to format the backtrace
    PyTracebackObject* ptb = (PyTracebackObject*)tb;
    while ( ptb != NULL ) {
        // Filename
        stream << "File \"" << PyBytes_AsString(ptb->tb_frame->f_code->co_filename) << "\", ";
        // Line number
        stream << "line " << ptb->tb_lineno << ", ";
        // Module name
        stream << PyBytes_AsString(ptb->tb_frame->f_code->co_name) << "\n";
        
        // Get the next line
        ptb = ptb->tb_next;
    }

    // Add in the other error information
    stream << exc_name << ": " << PyBytes_AsString(PyObject_Str(val)) << "\n";

    Simulation::getSimulationOutput().fatal(line, file, func, exit_code, "%s\n", stream.str().c_str());

}



SSTElementPythonModuleCode* 
SSTElementPythonModuleCode::addSubModule(const std::string& module_name, char* code, std::string filename)
{
    auto ret = new SSTElementPythonModuleCode(this,module_name,code,filename);
    sub_modules.push_back(ret);
    return ret;
}


void*
SSTElementPythonModuleCode::load(void* parent_module)
{
    PyObject* pm = (PyObject*)parent_module;

    PyObject *compiled_code = Py_CompileString(code, filename.c_str(), Py_file_input);
    if ( compiled_code == NULL) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running Py_CompileString on %s.  Details follow:\n", filename.c_str());

    PyObject *module = PyImport_ExecCodeModule(const_cast<char*>(getFullModuleName().c_str()), compiled_code);
    if ( module == NULL) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running PyImport_ExecCodeModule on %s.  Details follow:\n",filename.c_str());

    // All but the top level module need to add themselves to the top level module
    if ( parent != NULL) PyModule_AddObject(pm, getFullModuleName().c_str(), module);
    else pm = module;
    
    for ( auto item : sub_modules ) {
        item->load((void*)pm);
    }    
    
    return module;
}


std::string
SSTElementPythonModuleCode::getFullModuleName()
{
    if ( parent == NULL ) return module_name;
    return parent->getFullModuleName() + "." + module_name;
}




SSTElementPythonModule::SSTElementPythonModule(std::string library) :
    library(library),
    primary_module(NULL),
    primary_code_module(NULL)
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
    // Need to see if we're using the new hierarchy based method or the old
    if ( primary_code_module != NULL ) return primary_code_module->load(NULL);

    if ( primary_module == NULL ) {
        Simulation::getSimulationOutput().fatal(CALL_INFO,1,"SSTElementPythonModule: Primary module not set.  Use addPrimaryModule().\n");
    }
    PyObject *code = Py_CompileString(primary_module, pylibrary.c_str(), Py_file_input);
    if ( code == NULL) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running Py_CompileString on %s.  Details follow:\n",const_cast<char*>(pylibrary.c_str()));

    PyObject *module = PyImport_ExecCodeModule(const_cast<char*>(sstlibrary.c_str()), code);
    if ( module == NULL) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running PyImport_ExecCodeModule on %s.  Details follow:\n",const_cast<char*>(sstlibrary.c_str()));

    for ( auto item : sub_modules ) {
        std::string pylib = pylibrary + "-" + item.first;
        std::string sstlib = sstlibrary + "." + item.first;

        PyObject* subcode = Py_CompileString(item.second, pylib.c_str(), Py_file_input);
        if ( subcode == NULL) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running Py_CompileString on %s.  Details follow:\n",const_cast<char*>(pylib.c_str()));

        PyObject* submodule = PyImport_ExecCodeModule(const_cast<char*>(sstlib.c_str()), subcode);
        if ( submodule == NULL) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running PyImport_ExecCodeModule on %s.  Details follow:\n",const_cast<char*>(sstlib.c_str()));
        PyModule_AddObject(module, item.first.c_str(), submodule);
    }
    
    return module;
}


// New functions
SSTElementPythonModuleCode*
SSTElementPythonModule::createPrimaryModule(char* code, std::string filename)
{
    if ( primary_module == NULL ) {
        primary_code_module = new SSTElementPythonModuleCode(NULL, sstlibrary, code, filename);
    }
    else {
        Simulation::getSimulationOutput().fatal(CALL_INFO,1,"SSTElementPythonModule::createPrimaryModule: Attempt to create second primary module.\n");
    }
    return primary_code_module;
}


} // namespace SST

