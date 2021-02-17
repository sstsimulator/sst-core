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

#include "sst_config.h"

#include "warnmacros.h"
DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include "sst/core/model/element_python.h"

// Python header files
#include <traceback.h>
#include <frameobject.h>

#include "sst/core/output.h"
#include "sst/core/simulation.h"

namespace SST {

char empty_code[] = {
    0x00};


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
    char* exc_name = (char*) PyExceptionClass_Name(exc);
    if (exc_name != nullptr) {
        char *dot = strrchr(exc_name, '.');
        if (dot != nullptr)
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
    while ( ptb != nullptr ) {
        // Filename
#ifdef SST_CONFIG_HAVE_PYTHON3
        stream << "File \"" << PyUnicode_AsUTF8(ptb->tb_frame->f_code->co_filename) << "\", ";
#else
        stream << "File \"" << PyString_AsString(ptb->tb_frame->f_code->co_filename) << "\", ";
#endif
        // Line number
        stream << "line " << ptb->tb_lineno << ", ";
        // Module name
#ifdef SST_CONFIG_HAVE_PYTHON3
        stream << PyUnicode_AsUTF8(ptb->tb_frame->f_code->co_name) << "\n";
#else
        stream << PyString_AsString(ptb->tb_frame->f_code->co_name) << "\n";
#endif

        // Get the next line
        ptb = ptb->tb_next;
    }

    // Add in the other error information
#ifdef SST_CONFIG_HAVE_PYTHON3
    stream << exc_name << ": " << PyUnicode_AsUTF8(PyObject_Str(val)) << "\n";
#else
    stream << exc_name << ": " << PyString_AsString(PyObject_Str(val)) << "\n";
#endif
    Simulation::getSimulationOutput().fatal(line, file, func, exit_code, "%s\n", stream.str().c_str());

}



SSTElementPythonModuleCode*
SSTElementPythonModuleCode::addSubModule(const std::string& module_name, char* code, const std::string& filename)
{
    auto ret = new SSTElementPythonModuleCode(this,module_name,code,filename);
    sub_modules.push_back(ret);
    return ret;
}

SSTElementPythonModuleCode*
SSTElementPythonModuleCode::addSubModule(const std::string& module_name)
{
    auto ret = new SSTElementPythonModuleCode(this,module_name,empty_code,"empty_module");
    sub_modules.push_back(ret);
    return ret;
}


void*
SSTElementPythonModuleCode::load(void* parent_module)
{
    PyObject* pm = (PyObject*)parent_module;
    PyObject *compiled_code = Py_CompileString(code, filename.c_str(), Py_file_input);
    if ( compiled_code == nullptr) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running Py_CompileString on %s.  Details follow:\n", filename.c_str());

    PyObject *module = PyImport_ExecCodeModule(const_cast<char*>(getFullModuleName().c_str()), compiled_code);
    if ( module == nullptr) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running PyImport_ExecCodeModule on %s.  Details follow:\n",filename.c_str());

    // All but the top level module need to add themselves to the top level module
    if ( parent != nullptr) PyModule_AddObject(pm, getFullModuleName().c_str(), module);
    else pm = module;

    for ( auto item : sub_modules ) {
        item->load((void*)pm);
    }

    return module;
}


std::string
SSTElementPythonModuleCode::getFullModuleName()
{
    if ( parent == nullptr ) return module_name;
    return parent->getFullModuleName() + "." + module_name;
}




SSTElementPythonModule::SSTElementPythonModule(const std::string& library) :
    library(library),
    primary_module(nullptr),
    primary_code_module(nullptr)
{
    pylibrary = "py" + library;
    sstlibrary = "sst." + library;
}


void*
SSTElementPythonModule::load()
{
    // Need to see if we're using the new hierarchy based method or the old
    if ( primary_code_module != nullptr ) return primary_code_module->load(nullptr);

    if ( primary_module == nullptr ) {
        Simulation::getSimulationOutput().fatal(CALL_INFO,1,"SSTElementPythonModule: Primary module not set.  Use addPrimaryModule().\n");
    }
    PyObject *code = Py_CompileString(primary_module, pylibrary.c_str(), Py_file_input);
    if ( code == nullptr) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running Py_CompileString on %s.  Details follow:\n",const_cast<char*>(pylibrary.c_str()));

    PyObject *module = PyImport_ExecCodeModule(const_cast<char*>(sstlibrary.c_str()), code);
    if ( module == nullptr) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running PyImport_ExecCodeModule on %s.  Details follow:\n",const_cast<char*>(sstlibrary.c_str()));

    for ( auto item : sub_modules ) {
        std::string pylib = pylibrary + "-" + item.first;
        std::string sstlib = sstlibrary + "." + item.first;

        PyObject* subcode = Py_CompileString(item.second, pylib.c_str(), Py_file_input);
        if ( subcode == nullptr) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running Py_CompileString on %s.  Details follow:\n",const_cast<char*>(pylib.c_str()));

        PyObject* submodule = PyImport_ExecCodeModule(const_cast<char*>(sstlib.c_str()), subcode);
        if ( submodule == nullptr) abortOnPyErr(CALL_INFO,1,"SSTElementPythonModule: Error running PyImport_ExecCodeModule on %s.  Details follow:\n",const_cast<char*>(sstlib.c_str()));
        PyModule_AddObject(module, item.first.c_str(), submodule);
    }

    return module;
}


// New functions
SSTElementPythonModuleCode*
SSTElementPythonModule::createPrimaryModule(char* code, const std::string& filename)
{
    if ( primary_module == nullptr ) {
        primary_code_module = new SSTElementPythonModuleCode(nullptr, sstlibrary, code, filename);
    }
    else {
        Simulation::getSimulationOutput().fatal(CALL_INFO,1,"SSTElementPythonModule::createPrimaryModule: Attempt to create second primary module.\n");
    }
    return primary_code_module;
}

SSTElementPythonModuleCode*
SSTElementPythonModule::createPrimaryModule()
{
    if ( primary_module == nullptr ) {
        primary_code_module = new SSTElementPythonModuleCode(nullptr, sstlibrary, empty_code, "empty_module");
    }
    else {
        Simulation::getSimulationOutput().fatal(CALL_INFO,1,"SSTElementPythonModule::createPrimaryModule: Attempt to create second primary module.\n");
    }
    return primary_code_module;
}


} // namespace SST

