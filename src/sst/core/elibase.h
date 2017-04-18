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

#ifndef SST_CORE_ELIBASE_H
#define SST_CORE_ELIBASE_H

#include <sst/core/sst_types.h>

#include <string>
#include <vector>

// Component Category Definitions
#define COMPONENT_CATEGORY_UNCATEGORIZED  0x00
#define COMPONENT_CATEGORY_PROCESSOR      0x01
#define COMPONENT_CATEGORY_MEMORY         0x02
#define COMPONENT_CATEGORY_NETWORK        0x04
#define COMPONENT_CATEGORY_SYSTEM         0x08

namespace SST {

/** Describes Statistics used by a Component.
 */
struct ElementInfoStatistic {
    const char* name;		/*!< Name of the Statistic to be Enabled */
    const char* description;	/*!< Brief description of the Statistic */
    const char* units;          /*!< Units associated with this Statistic value */
    const uint8_t enableLevel;	/*!< Level to meet to enable statistic 0 = disabled */
};

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

/** Describes Ports that the Component can use
 */
struct ElementInfoPort2 {
    const char *name;			/*!< Name of the port.  Can contain %d for a dynamic port, also %(xxx)d for dynamic port with xxx being the controlling component parameter */
    const char *description;	/*!< Brief description of the port (ie, what it is used for) */
    const std::vector<std::string> validEvents;	/*!< List of fully-qualified event types that this Port expects to send or receive */
};

struct ElementInfoSubComponentSlot {
    const char * name;
    const char * description;
    const char * superclass;
};

typedef ElementInfoSubComponentSlot ElementInfoSubComponentHook;

} //namespace SST

#endif // SST_CORE_ELIBASE_H
