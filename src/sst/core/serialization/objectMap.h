// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_OBJECTMAP_H
#define SST_CORE_SERIALIZATION_OBJECTMAP_H

#include "sst/core/from_string.h"
#include "sst/core/warnmacros.h"

#include <string>
#include <vector>

namespace SST {
namespace Core {
namespace Serialization {

class ObjectMap;

// Metadata object that each ObjectMap has a pointer to in order to
// track the hierarchy information while traversing the data
// structures.  This is used because a given object can be pointed to
// by multiple other objects, so we need to track the "path" through
// which we got to the object so we can traverse back up the object
// hierarchy.
struct ObjectMapMetaData
{
    /**
       Parent object that contained this object and through which it
       was selected.
    */
    ObjectMap* parent;

    /**
       Name of this object in the context of the currently set parent
     */
    std::string name;

    /**
       Constructor for intializing data memebers
     */
    ObjectMapMetaData(ObjectMap* parent, const std::string& name) : parent(parent), name(name) {}
};

/**
   Class created by the serializer mapping mode used to map the
   variables for objects.  This allows access to read and write the
   mapped variables.  The base class is used for non-fundamental and
   non-container types, but there is a templated child class used for
   fundameentals and containers.  The templating is needed so that the
   type information can be embedded in the code for reading and
   writing.
 */
class ObjectMap
{
protected:
    /**
       Static empty variable vector for use by versions that don't
       have variables (i.e. are fundamentals or classes treated as
       fundamentals.  This is needed because getVariables() returns a
       reference to the vector.
    */
    static std::vector<std::pair<std::string, ObjectMap*>> emptyVars;

    /**
       Metedata object for walking the object hierarchy.  When this
       object is selected by a parent object, a metadata object will
       be added.  The metadata contains a pointer to the parent and
       the name of this object in the context of the parent. If this
       object is selected and the metadata is non a nullptr, then we
       have hit a loop in the data structure.

       Under normal circumstances, the metadata allows you to get the
       full path of the object according to how you walked the
       hierarchy, and allows you to return to the parent object.  If a
       loop is detected on select, then the full path of the object
       will return to the highest level path and the metadata from
       that path to the current path will be erased.
     */
    ObjectMapMetaData* mdata_ = nullptr;


    /**
       Indicates wheter or not the variable is read-only
     */
    bool read_only_ = false;


    /**
       Function implemented by derived classes to implement set(). No
       need to check for read_only, that is done in set().
    */
    virtual void set_impl(const std::string& UNUSED(value)) {}

    /**
       Function that will get called when this object is selected
     */
    virtual void activate_callback() {}
    virtual void deactivate_callback() {}

private:
    int32_t refCount_ = 1;

public:
    /**
       Default constructor primarily used for the "top" object in the hierarchy
     */
    ObjectMap() {}


    inline bool isReadOnly() { return read_only_; }
    inline void setReadOnly() { read_only_ = true; }


    /**
       Get the name of the variable represented by this ObjectMap

       @return Name of variable
     */
    std::string getName();


    /**
       Get the full hierarchical name of the variable represented by
       this ObjectMap, based on the path taken to get to this object.

       @return Full hierarchical name of variable
     */
    std::string getFullName();


    /**
       Get the type of the variable represented by the ObjectMap

       @return Type of variable
     */
    virtual std::string getType() = 0;

    /**
       Get the address of the variable represented by the ObjectMap

       @return Address of varaible
     */
    virtual void* getAddr() = 0;


    /**
       Get the list of child variables contained in this ObjectMap

       @return Reference to vector containing ObjectMaps for this
       ObjectMap's child variables. Fundamental types will return the
       same empty vector.
     */
    virtual const std::vector<std::pair<std::string, ObjectMap*>>& getVariables() { return emptyVars; }


    /**
       Increament the reference counter for this object map
     */
    void incRefCount() { refCount_++; }

    /**
       Decreament the reference counter for this object map.  If this
       reference count reaches zero, the object will delete itself.
       NOTE: delete should not be called directly on this object and
       should only be done automatically using decRefCount().
     */
    void decRefCount()
    {
        refCount_--;
        if ( refCount_ == 0 ) { delete this; }
    }

    /**
       Get the current reference count

       @return current value of reference counter for the object
     */
    int32_t getRefCount() { return refCount_; }


    /************ Functions for walking the Object Hierarchy ************/


    /**
       Get the parent for this ObjectMap

       @return Parent for this ObjectMap. If this is the top of the
       hierarchy, it will return nullptr
     */
    ObjectMap* selectParent();

    /**
       Get the ObjectMap for the specified variable

       @param name Name of variable to select

       @return ObjectMap for specified variable, if it exists, this
       otherwise
    */
    ObjectMap* selectVariable(std::string name, bool& loop_detected);


    /**
       Adds a varaible to this ObjectMap.  NOTE: calls to this
       function will be ignore if isFundamental() returns true.

       @param name Name of the object in the context of the parent class

       @param obj ObjectMap to add as a variable

     */
    virtual void addVariable(const std::string& UNUSED(name), ObjectMap* UNUSED(obj)) {}


    /************ Functions for getting/setting Object's Value *************/

    /**
       Get the value of the variable as a string.  NOTE: this function
       is only valid for ObjectMaps that represent fundamental types
       or classes treated as fundamental types.

       @return Value of the represented variable as a string
     */
    virtual std::string get() { return ""; }

    /**
       Sets the value of the variable represented by the ObjectMap to
       the specified value, which is represented as a string.  The
       templated child classes for fundamentals will know how to
       convert the string to a value of the approproprite type.  NOTE:
       this fucntion is only value for ObjectMaps that represent
       fundamental types or classes treated as fundamentatl types.

       @param value Value to set the object to represented as a string

    */
    void set(const std::string& value)
    {
        if ( read_only_ )
            return;
        else
            set_impl(value);
    }

    /**
       Gets the value of the specified variable as a string.  NOTE:
       this function is only valid for ObjectMaps that represent
       non-fundamental types or classes not treated as fundamental
       types.

       @param var Name of variable

       @return Value of the specified variable as a string
     */
    virtual std::string get(const std::string& var);

    /**
       Sets the value of the specified variable to the specified
       value, which is represented as a string.  The templated child
       classes for fundamentals will know how to convert the string to
       a value of the approproprite type.  NOTE: this fucntion is only
       valuid for ObjectMaps that represent non-fundamental types or
       classes not treated as fundamentatl types (i.e. they must have
       childrent).

       @param var Name of variable

       @param value Value to set var to represented as a string

       @param[out] found Set to true if  var is found, set to false otherwise

       @param[out] read_only Set to true if var is read-only, set to false otherwise

    */
    virtual void set(const std::string& var, const std::string& value, bool& found, bool& read_only);

    /**
       Check to see if this ObjectMap represents a fundamental or a
       class treated as a fundamental.

       @return true if this ObjectMap represents a fundamental or
       class treated as a fundamental, false otherwise
     */
    virtual bool isFundamental() { return false; }

    /**
       Check to see if this ObjectMap represents a container

       @return true if this ObjectMap represents a container, false
       otherwise
     */
    virtual bool isContainer() { return false; }


    /**
       Destructor
     */
    virtual ~ObjectMap() {}

    /**
       Static function to demangle type names returned from typeid<T>.name()

       @param name typename returned from typid<T>.name()

       @return demangled name
     */
    static std::string demangle_name(const char* name);

    /**
       Create a string that lists information for the specified
       variable.  This will list all child variables, including the
       values for any children that are fundamentals.

       @param name name of variable to list

       @param[out] found Set to true if variable is found, set to false otherwise

       @param recurse number of levels to recurse (default is 0)

       @return String representing this object and any children
       included based on the value of recurse
     */
    virtual std::string listVariable(std::string name, bool& found, int recurse = 0);

    /**
       Create a string that lists information for the current object.
       This will list all child variables, including the values for
       any children that are fundamentals.

       @param recurse number of levels to recurse (default is 0)

       @return String representing this object and any children
       included based on the value of recurse
     */
    virtual std::string list(int recurse = 0);


private:
    inline void activate(ObjectMap* parent, const std::string& name)
    {
        mdata_ = new ObjectMapMetaData(parent, name);
        activate_callback();
    }

    inline void deactivate()
    {
        delete mdata_;
        mdata_ = nullptr;
        deactivate_callback();
    }

    inline ObjectMap* findVariable(const std::string& name)
    {
        const std::vector<std::pair<std::string, ObjectMap*>>& variables = getVariables();
        for ( auto& x : variables ) {
            if ( x.first == name ) { return x.second; }
        }
        return nullptr;
    }

    std::string listRecursive(const std::string& name, int level, int recurse);
};


/**
   ObjectMap object for non-fundamental, non-container types.  This
   class allows for child variables.
 */
class ObjectMapWithChildren : public ObjectMap
{
protected:
    /**
       Vector that child ObjectMaps are stored in
     */
    std::vector<std::pair<std::string, ObjectMap*>> variables_;

    ObjectMapWithChildren() : ObjectMap() {}

public:
    ~ObjectMapWithChildren()
    {
        for ( auto obj : variables_ ) {
            if ( obj.second != nullptr ) obj.second->decRefCount();
        }
        variables_.clear();
    }

    /**
       Adds a variable to this ObjectMap

       @param obj ObjectMap to add as a variable
     */
    void addVariable(const std::string& name, ObjectMap* obj) override
    {
        variables_.push_back(std::make_pair(name, obj));
    }


    /**
       Get the list of child variables contained in this ObjectMap

       @return Refernce to vector containing ObjectMaps for this
       ObjectMap's child variables. pair.first is the name of the
       variable in the context of this object.
     */
    const std::vector<std::pair<std::string, ObjectMap*>>& getVariables() override { return variables_; }
};


/**
   ObjectMap object to create a level of hierarchy that doesn't
   represent a specific object.  This can be used to create views of
   data that don't align specifically with the underlying data
   structures.
 */
class ObjectMapHierarchyOnly : public ObjectMapWithChildren
{
public:
    ObjectMapHierarchyOnly() : ObjectMapWithChildren() {}

    ~ObjectMapHierarchyOnly() {}

    /**
       Returns empty string since there is no underlying object being
       represented

       @return empty string
     */
    std::string getType() override { return ""; }

    /**
       Returns nullptr since there is no underlying object being
       represented

       @return nullptr
     */
    void* getAddr() override { return nullptr; }
};


/**
   ObjectMap object for non-fundamental, non-container types.  This
   class allows for child variables.
 */
class ObjectMapClass : public ObjectMapWithChildren
{
protected:
    /**
       Type of the variable as given by the demangled version of
       typeif<T>.name() for the type.
     */
    std::string type_;

    /**
       Address of the variable for reading and writing
     */
    void* addr_ = nullptr;

public:
    ObjectMapClass() : ObjectMapWithChildren() {}

    ObjectMapClass(void* addr, const std::string& type) :
        ObjectMapWithChildren(),
        type_(demangle_name(type.c_str())),
        addr_(addr)
    {}

    ~ObjectMapClass() {}

    /**
       Get the type of the represented object

       @return type of represented object
     */
    std::string getType() override { return type_; }

    /**
       Get the address of the represented object

       @return address of represented object
     */
    void* getAddr() override { return addr_; }
};


/**
   ObjectMap object fundamental types, and classes treated as
   fundamental types.  In order for an object to be treated as a
   fundamental, it must be printable using std::to_string() and
   assignable using SST::Core::from_string().
*/
template <typename T>
class ObjectMapFundamental : public ObjectMap
{
protected:
    /**
       Address of the variable for reading and writing
     */
    T* addr_ = nullptr;

public:
    virtual std::string get() override { return std::to_string(*addr_); }
    virtual void        set_impl(const std::string& value) override { *addr_ = SST::Core::from_string<T>(value); }

    bool isFundamental() override { return true; }

    /**
       Get the address of the variable represented by the ObjectMap

       @return Address of varaible
     */
    void* getAddr() override { return addr_; }


    /**
       Get the list of child variables contained in this ObjectMap,
       which in this case will be empty.

       @return Refernce to vector containing ObjectMaps for this
       ObjectMap's child variables. This vector will be empty because
       fundamentals have no children
     */
    const std::vector<std::pair<std::string, ObjectMap*>>& getVariables() override { return emptyVars; }

    ObjectMapFundamental(T* addr) : ObjectMap(), addr_(addr) {}

    ~ObjectMapFundamental() {}

    std::string getType() override { return demangle_name(typeid(T).name()); }
};


} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_OBJECTMAP_H
