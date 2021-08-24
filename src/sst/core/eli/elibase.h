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

#ifndef SST_CORE_ELI_ELIBASE_H
#define SST_CORE_ELI_ELIBASE_H

#include "sst/core/sst_types.h"

#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Component Category Definitions
#define COMPONENT_CATEGORY_UNCATEGORIZED 0x00
#define COMPONENT_CATEGORY_PROCESSOR     0x01
#define COMPONENT_CATEGORY_MEMORY        0x02
#define COMPONENT_CATEGORY_NETWORK       0x04
#define COMPONENT_CATEGORY_SYSTEM        0x08

namespace SST {

/** Describes Statistics used by a Component.
 */
struct ElementInfoStatistic
{
    const char*   name;        /*!< Name of the Statistic to be Enabled */
    const char*   description; /*!< Brief description of the Statistic */
    const char*   units;       /*!< Units associated with this Statistic value */
    const uint8_t enableLevel; /*!< Level to meet to enable statistic 0 = disabled */
};

/** Describes Parameters to a Component.
 */
struct ElementInfoParam
{
    const char* name;         /*!< Name of the parameter */
    const char* description;  /*!< Brief description of the parameter (ie, what it controls) */
    const char* defaultValue; /*!< Default value (if any) nullptr == required parameter with no default, "" == optional
                                 parameter, blank default, "foo" == default value */
};

/** Describes Ports that the Component can use
 */
struct ElementInfoPort
{
    const char* name; /*!< Name of the port.  Can contain %d for a dynamic port, also %(xxx)d for dynamic port with xxx
                         being the controlling component parameter */
    const char* description; /*!< Brief description of the port (ie, what it is used for) */
    const std::vector<std::string>
        validEvents; /*!< List of fully-qualified event types that this Port expects to send or receive */
};

struct ElementInfoSubComponentSlot
{
    const char* name;
    const char* description;
    const char* superclass;
};

typedef ElementInfoSubComponentSlot ElementInfoSubComponentHook;

namespace ELI {

// Function used to combine the parent and child ELI information.
// This function only works for items that have a 'name' and a
// 'description' field (which is all of them at time of function
// writing).  You can delete a parent't info by adding an entry in the
// child with the same name and a nullptr in the description.  Each
// info item should include a macro of the format SST_ELI_DELETE_* to
// make this easy for the element library writer.
//
// The function creates a new vector and the uses vector::swap because
// the ElementInfo* classes have const data members so deleting from
// the vector does not compile (copy constructor is deleted).
template <typename T>
static void
combineEliInfo(std::vector<T>& base, std::vector<T>& add)
{
    std::vector<T> combined;
    // Add in any item that isn't already defined
    for ( auto x : add ) {
        bool add = true;
        for ( auto y : base ) {
            if ( !strcmp(x.name, y.name) ) {
                add = false;
                break;
            }
        }
        if ( add ) combined.emplace_back(x);
    }

    // Now add all the locals.  We will skip any one that has nullptr
    // in the description field
    for ( auto x : base ) {
        if ( x.description != nullptr ) { combined.emplace_back(x); }
    }
    base.swap(combined);
}

template <class T>
struct MethodDetect
{
    using type = void;
};

struct LibraryLoader
{
    virtual void load() = 0;
    virtual ~LibraryLoader() {}
};

class LoadedLibraries
{
public:
    using InfoMap    = std::map<std::string, std::list<LibraryLoader*>>;
    using LibraryMap = std::map<std::string, InfoMap>;

    static bool isLoaded(const std::string& name);

    /**
       @return A boolean indicated successfully added
    */
    static bool addLoader(const std::string& lib, const std::string& name, LibraryLoader* loader);

    static const LibraryMap& getLoaders();

private:
    static std::unique_ptr<LibraryMap> loaders_;
};

} // namespace ELI
} // namespace SST

#define ELI_FORWARD_AS_ONE(...) __VA_ARGS__

// This is the macro used to declare a class that will become the base
// class for classes that are to be loaded through the ELI.

// The __EliBaseLevel and __EliDerivedLevel are used to determine the
// ELI parent API.  If __EliDerivedLevel is greater than
// __EliBaseLevel, then the parent API type is stored in
// __LocalEliBase.  If not, then the parent API is __ParentEliBase.
// In other words, the parent API is __LocalEliBase if, and only if,
// the class is a derived class (i.e., calls SST_ELI_REGISTER_DERIVED
// either directly or indirectly) and has not called
// SST_ELI_DECLARE_BASE or SST_ELI_DECLARE_NEW_BASE.  If the class has
// called either of the *_BASE macros, then they are APIs and if you
// derive a class from it, then you need to use that class also as the
// ELI parent.
#define SST_ELI_DECLARE_BASE(Base)                 \
    using __LocalEliBase                   = Base; \
    using __ParentEliBase                  = void; \
    static constexpr int __EliBaseLevel    = 0;    \
    static constexpr int __EliDerivedLevel = 0;    \
    static const char*   ELI_baseName() { return #Base; }

#define SST_ELI_DECLARE_INFO_COMMON()                                           \
    using InfoLibrary = ::SST::ELI::InfoLibrary<__LocalEliBase>;                \
    template <class __TT>                                                       \
    static bool addDerivedInfo(const std::string& lib, const std::string& elem) \
    {                                                                           \
        return addInfo(lib, elem, new BuilderInfo(lib, elem, (__TT*)nullptr));  \
    }

// This macro can be used to declare a new base class that inherits
// from another base class.  The ELI information will also be
// inherited (added to whatever the child class declares).  Items in
// the child will overwrite items in the parent.  See comment for
// combineEliInfo() for information about deleting items from the
// parent API.
#define SST_ELI_DECLARE_NEW_BASE(OldBase, NewBase)                                           \
    using __LocalEliBase                = NewBase;                                           \
    using __ParentEliBase               = OldBase;                                           \
    static constexpr int __EliBaseLevel = OldBase::__EliBaseLevel + 2;                       \
    SST_ELI_DECLARE_INFO_COMMON()                                                            \
    static const char* ELI_baseName() { return #NewBase; }                                   \
    template <class InfoImpl>                                                                \
    static bool addInfo(const std::string& elemlib, const std::string& elem, InfoImpl* info) \
    {                                                                                        \
        return OldBase::addInfo(elemlib, elem, info) &&                                      \
               ::SST::ELI::InfoDatabase::getLibrary<NewBase>(elemlib)->addInfo(elem, info);  \
    }

#endif // SST_CORE_ELI_ELIBASE_H
