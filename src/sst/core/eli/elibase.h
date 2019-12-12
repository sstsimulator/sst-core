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

#ifndef SST_CORE_ELIBASE_H
#define SST_CORE_ELIBASE_H

#include "sst/core/sst_types.h"

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <functional>
#include <memory>

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
    const char* name;        /*!< Name of the Statistic to be Enabled */
    const char* description;    /*!< Brief description of the Statistic */
    const char* units;          /*!< Units associated with this Statistic value */
    const uint8_t enableLevel;    /*!< Level to meet to enable statistic 0 = disabled */
};

/** Describes Parameters to a Component.
 */
struct ElementInfoParam {
    const char *name;            /*!< Name of the parameter */
    const char *description;    /*!< Brief description of the parameter (ie, what it controls) */
    const char *defaultValue;    /*!< Default value (if any) nullptr == required parameter with no default, "" == optional parameter, blank default, "foo" == default value */
};

/** Describes Ports that the Component can use
 */
struct ElementInfoPort {
    const char *name;            /*!< Name of the port.  Can contain %d for a dynamic port, also %(xxx)d for dynamic port with xxx being the controlling component parameter */
    const char *description;    /*!< Brief description of the port (ie, what it is used for) */
    const char **validEvents;    /*!< List of fully-qualified event types that this Port expects to send or receive */
};

/** Describes Ports that the Component can use
 */
struct ElementInfoPort2 {
    const char *name;            /*!< Name of the port.  Can contain %d for a dynamic port, also %(xxx)d for dynamic port with xxx being the controlling component parameter */
    const char *description;    /*!< Brief description of the port (ie, what it is used for) */
    const std::vector<std::string> validEvents;    /*!< List of fully-qualified event types that this Port expects to send or receive */

    // For backwards compatibility, convert from ElementInfoPort to ElementInfoPort2
private:
    std::vector<std::string> createVector(const char** events) {
        std::vector<std::string> vec;
        if ( events == nullptr ) return vec;
        const char** ev = events;
        while ( nullptr != *ev ) {
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

struct LibraryLoader {
  virtual void load() = 0;
  virtual ~LibraryLoader(){}
};

class LoadedLibraries {
public:
    using InfoMap=std::map<std::string,std::list<LibraryLoader*>>;
    using LibraryMap=std::map<std::string,InfoMap>;

    static bool isLoaded(const std::string& name);

    /**
       @return A boolean indicated successfully added
    */
    static bool addLoader(const std::string& lib, const std::string& name,
                          LibraryLoader* loader);

    static const LibraryMap& getLoaders();

private:
    static std::unique_ptr<LibraryMap> loaders_;

};

} //namespace ELI
} //namespace SST

#define ELI_FORWARD_AS_ONE(...) __VA_ARGS__

#define SST_ELI_DECLARE_BASE(Base) \
  using __LocalEliBase = Base; \
  static const char* ELI_baseName(){ return #Base; }

#define SST_ELI_DECLARE_INFO_COMMON()                          \
  using InfoLibrary = ::SST::ELI::InfoLibrary<__LocalEliBase>; \
  template <class __TT> static bool addDerivedInfo(const std::string& lib, const std::string& elem){ \
    return addInfo(lib,elem,new BuilderInfo(lib,elem,(__TT*)nullptr)); \
  }

#define SST_ELI_DECLARE_NEW_BASE(OldBase,NewBase) \
  using __LocalEliBase = NewBase; \
  using __ParentEliBase = OldBase; \
  SST_ELI_DECLARE_INFO_COMMON() \
  static const char* ELI_baseName(){ return #NewBase; } \
  template <class InfoImpl> static bool addInfo(const std::string& elemlib, const std::string& elem, \
                                                InfoImpl* info){ \
    return OldBase::addInfo(elemlib, elem, info) \
      && ::SST::ELI::InfoDatabase::getLibrary<NewBase>(elemlib)->addInfo(elem,info); \
  }


#endif // SST_CORE_ELIBASE_H
