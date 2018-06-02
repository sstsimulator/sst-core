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


#ifndef SST_CORE_MODEL_ELEMENT_PYTHON_H
#define SST_CORE_MODEL_ELEMENT_PYTHON_H

#include <vector>
#include <string>

namespace SST {

typedef void* (*genPythonModuleFunction)(void);


/** Base class for python modules in element libraries
 */
class SSTElementPythonModule {

protected:
    std::string library;
    std::string pylibrary;
    std::string sstlibrary;
    char* primary_module;

    std::vector<std::pair<std::string,char*> > sub_modules;

    // Only needed for supporting the old ELI
    SSTElementPythonModule() {}
    
public:
    virtual ~SSTElementPythonModule() {}
    
    SSTElementPythonModule(std::string library);

    void addPrimaryModule(char* file);
    void addSubModule(std::string name, char* file);

    virtual void* load();
};

// Class to use to support old ELI
class SSTElementPythonModuleOldELI : public SSTElementPythonModule {
private:
    genPythonModuleFunction func;
    
public:
    SSTElementPythonModuleOldELI(genPythonModuleFunction func) :
        SSTElementPythonModule(),
        func(func)
        {
        }

    void* load() override {
        return (*func)();
    }
};

}

#endif // SST_CORE_MODEL_ELEMENT_PYTHON_H
