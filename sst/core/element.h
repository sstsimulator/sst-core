// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ELEMENT_H
#define SST_ELEMENT_H

#include <sst/core/component.h>
#include <sst/core/params.h>

namespace SST {
class Introspector;
class ConfigGraph;
class Subcomponent;
//class Module;
 
typedef Component* (*componentAllocate)(ComponentId_t, Params&);
typedef Introspector* (*introspectorAllocate)(Params&);
typedef void (*eventInitialize)(void);
typedef Subcomponent* (*subcomponentLoad)(Component*, Params&);
//typedef Module* (*moduleLoad)(Params&);
typedef void (*partitionFunction)(ConfigGraph*,int);
typedef void (*generateFunction)(ConfigGraph*, std::string options, int ranks);
 
struct ElementInfoComponent {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    componentAllocate alloc;
};

struct ElementInfoIntrospector {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    introspectorAllocate alloc;
};

struct ElementInfoEvent {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    eventInitialize init;
};

struct ElementInfoSubcomponent {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    subcomponentLoad func;
};

//struct ElementInfoModule {
//    const char *name;
//    const char *description;
//    void (*printHelp)(FILE *output);
//    moduleLoad func;
//};

struct ElementInfoPartitioner {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    partitionFunction func;
};

struct ElementInfoGenerator {
    const char *name;
    const char *description;
    void (*printHelp)(FILE *output);
    generateFunction func;
};

struct ElementLibraryInfo {
    const char *name;
    const char *description;
    const struct ElementInfoComponent* components;
    const struct ElementInfoEvent* events;
    const struct ElementInfoIntrospector* introspectors;
    /* const struct ElementInfoSubcomponent* subcomponents; */
//    const struct ElementInfoModule* modules;
    const struct ElementInfoPartitioner* partitioners;
    const struct ElementInfoGenerator* generators;
};
};

#endif // SST_ELEMENT_H
