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

#ifndef SST_CORE_ELIBASE_H
#define SST_CORE_ELIBASE_H

#include <sst/core/sst_types.h>

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <map>

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

    // For backwards compatibility, convert from ElementInfoPort to ElementInfoPort2
private:
    std::vector<std::string> createVector(const char** events) {
        std::vector<std::string> vec;
        if ( events == NULL ) return vec;
        const char** ev = events;
        while ( NULL != *ev ) {
            vec.push_back(*ev);
            ev++;
        }
        return vec;
    }

public:
    
    ElementInfoPort2(const ElementInfoPort* port) :
        name(port->name),
        description(port->description),
        validEvents(createVector(port->validEvents))
        {}

    ElementInfoPort2(const char* name, const char* description, const std::vector<std::string> validEvents) :
        name(name),
        description(description),
        validEvents(validEvents)
        {}
};

struct ElementInfoSubComponentSlot {
    const char * name;
    const char * description;
    const char * superclass;
};

typedef ElementInfoSubComponentSlot ElementInfoSubComponentHook;

namespace ELI {

template <class T> struct MethodDetect { using type=void; };

class LoadedLibraries {
 public:
  using InfoMap=std::map<std::string, std::function<void()>>;
  using LibraryMap=std::map<std::string,InfoMap>;
  static void addLoaded(const std::string& name);

  static bool isLoaded(const std::string& name);

  static void addLoader(const std::string& lib, const std::string& name,
                        std::function<void()>&& loader);

  static const LibraryMap& getLoaders(){
    return *loaders_;
  }

 private:
  static std::unique_ptr<std::set<std::string>> loaded_;
  static std::unique_ptr<LibraryMap> loaders_;


};

} //namespace ELI
} //namespace SST

#define ELI_FORWARD_AS_ONE(...) __VA_ARGS__

#endif // SST_CORE_ELIBASE_H
