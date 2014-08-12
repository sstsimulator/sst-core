// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_ELEMENT_H
#define SST_CORE_ELEMENT_H

#include <sst/core/sst_types.h>

//#include <sst/core/component.h>
//#include <sst/core/params.h>

#include <stdio.h>
#include <string>

// Component Category Definitions
#define COMPONENT_CATEGORY_UNCATEGORIZED 0x00
#define COMPONENT_CATEGORY_PROCESSOR      0x01
#define COMPONENT_CATEGORY_MEMORY         0x02
#define COMPONENT_CATEGORY_NETWORK        0x04
#define COMPONENT_CATEGORY_SYSTEM         0x08

namespace SST {
class Component;
class ConfigGraph;
class PartitionGraph;
class Introspector;
class Module;
class Params;

typedef Component* (*componentAllocate)(ComponentId_t, Params&);
typedef Introspector* (*introspectorAllocate)(Params&);
typedef void (*eventInitialize)(void);
typedef Module* (*moduleAllocate)(Params&);
typedef Module* (*moduleAllocateWithComponent)(Component*, Params&);
typedef void (*partitionFunction)(ConfigGraph*,int);
typedef void (*generateFunction)(ConfigGraph*, std::string options, int ranks);
typedef void* (*genPythonModuleFunction)(void);

/** Describes Parameters to a Component.
 */
struct ElementInfoParam {
    const char *name;			/*!< Name of the parameter */
    const char *description;	/*!< Brief description of the parameter (ie, what it controls) */
    const char *defaultValue;	/*!< Default value (if any) NULL == required parameter with no default, "" == optional parameter, blank default, "foo" == default value */
};

/** Describes Ports that the Component can use
 */
struct ElementInfoPort {
    const char *name;			/*!< Name of the port.  Can contain %d for a dynamic port, also %(xxx)d for dynamic port with xxx being the controlling component parameter */
    const char *description;	/*!< Brief description of the port (ie, what it is used for) */
    const char **validEvents;	/*!< List of fully-qualified event types that this Port expects to send or receive */
};

/** Describes a Component and its associated information
 */
struct ElementInfoComponent {
    const char *name;					/*!< Name of the component */
    const char *description;			/*!< Brief description of what the component does */
    void (*printHelp)(FILE *output);	/*!< Pointer to a function that will print additional documentation about the component. (optional) */
    componentAllocate alloc;			/*!< Pointer to function to allocate a new instance of this component. */
    const ElementInfoParam *params;		/*!< List of parameters for which this component expects to look. */
    const ElementInfoPort *ports;		/*!< List of ports that this component uses. */
    uint32_t category;					/*!< Bit-mask of categories in which this component fits. */
};

/** Describes an Introspector
 */
struct ElementInfoIntrospector {
    const char *name;					/*!< Name of the introspector */
    const char *description;			/*!< Brief description of what the introspector does. */
    void (*printHelp)(FILE *output);	/*!< Pointer to a function that will print additional documentation about the introspector. (optional) */
    introspectorAllocate alloc;			/*!< Pointer to a function to allocate a new instance of this introspector. */
    const ElementInfoParam *params;		/*!< List of parameters which this introspector uses. */
};

/** Describes an Event
 */
struct ElementInfoEvent {
    const char *name;					/*!< Name of the event */
    const char *description;			/*!< Brief description of this event. */
    void (*printHelp)(FILE *output);	/*!< Pointer to a function that will print additional documentation about the event. (optional) */
    eventInitialize init;				/*!< Pointer to a function to initialize the library for use of this event. (optional) */
};

/** Describes a Module
 */
struct ElementInfoModule {
    const char *name;								/*!< Name of the module. */
    const char *description;						/*!< Brief description of the module. */
    void (*printHelp)(FILE *output);				/*!< Pointer to a function that will print additional documentation about the module. (optional) */
    moduleAllocate alloc;							/*!< Pointer to a function to do a default initialization of the module. */
    moduleAllocateWithComponent alloc_with_comp;	/*!< Pointer to a function to initialize a module instance, passing a Component as an argument. */
    const ElementInfoParam *params;					/*!< List of parameters which are used by this module. */
    const char *provides;                           /*!< Name of SuperClass which for this module can be used. */
};

/** Describes a Partitioner
 */
struct ElementInfoPartitioner {
    const char *name;					/*!< Name of the Partitioner */
    const char *description;			/*!< Brief description of the partitioner */
    void (*printHelp)(FILE *output);	/*!< Pointer to a function that will print additional documentation about the partitioner. (optional) */
    partitionFunction func;				/*!< Function to be called to perform the paritioning */
};

/** Describes a Generator
 */
struct ElementInfoGenerator {
    const char *name;					/*!< Name of the Generator */
    const char *description;			/*!< Brief description of the generator */
    void (*printHelp)(FILE *output);	/*!< Pointer to a function that will print additional documentation about the generator. (optional) */
    generateFunction func;				/*!< Function to be called to perform the graph generation */
};

/** Describes all the parts of the Element Library
 */
struct ElementLibraryInfo {
    const char *name;										/*!< Name of the Library. */
    const char *description;								/*!< Brief description of the Library */
    const struct ElementInfoComponent* components;			/*!< List of Components contained in the library. */
    const struct ElementInfoEvent* events;					/*!< List of Events exported by the library. */
    const struct ElementInfoIntrospector* introspectors;	/*!< List of Introspectors provided by the library. */
    const struct ElementInfoModule* modules;				/*!< List of Modules provided by the library. */
    const struct ElementInfoPartitioner* partitioners;		/*!< List of Partitioners provided by the library. */
    const struct ElementInfoGenerator* generators;			/*!< List of Generators provided by the library. */
    genPythonModuleFunction pythonModuleGenerator;			/*!< Pointer to Function to generate a Python Module for use in Configurations */
};
} //namespace SST

#endif // SST_CORE_ELEMENT_H
