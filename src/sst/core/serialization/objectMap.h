// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

/**
   Metadata object that each ObjectMap has a pointer to in order to
   track the hierarchy information while traversing the data
   structures.  This is used because a given object can be pointed to
   by multiple other objects, so we need to track the "path" through
   which we got to the object so we can traverse back up the object
   hierarchy.
*/
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
   Base class for objects created by the serializer mapping mode used
   to map the variables for objects.  This allows access to read and
   write the mapped variables.  ObjectMaps for fundamental types are
   templated because they need the type information embedded in the
   code so they can read and print the values.
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
       Metadata object for walking the object hierarchy.  When this
       object is selected by a parent object, a metadata object will
       be added.  The metadata contains a pointer to the parent and
       the name of this object in the context of the parent. When the
       object selects its parent, then this field is set to nullptr.
       If this object is selected and the metadata is not a nullptr,
       then we have hit a loop in the data structure.

       Under normal circumstances, the metadata allows you to get the
       full path of the object according to how you walked the
       hierarchy, and allows you to return to the parent object.  If a
       loop is detected on select, then the full path of the object
       will return to the highest level path and the metadata from
       that path to the current path will be erased.
     */
    ObjectMapMetaData* mdata_ = nullptr;


    /**
       Indicates whether or not the variable is read-only
     */
    bool read_only_ = false;


    /**
       Function implemented by derived classes to implement set(). No
       need to check for read-only, that is done in set().

       @param value Value to set the object to expressed as a string
    */
    virtual void set_impl(const std::string& UNUSED(value)) {}

    /**
       Function that will get called when this object is selected
     */
    virtual void activate_callback() {}

    /**
       Function that will get called when this object is deactivated
       (i.e selectParent() is called)
     */
    virtual void deactivate_callback() {}

private:
    /**
       Reference counter so the object knows when to delete itself.
       ObjectMaps should not be deleted directly, rather the
       decRefCount() function should be called when the object is no
       longer needed and the object will delete itself if refCount_
       reaches 0.
     */
    int32_t refCount_ = 1;

public:
    /**
       Default constructor primarily used for the "top" object in the hierarchy
     */
    ObjectMap() {}


    /**
       Check to see if this object is read-only

       @return true if ObjectMap is read-only, false otherwise
     */
    inline bool isReadOnly() { return read_only_; }

    /**
       Set the read-only state of the object.  NOTE: If the ObjectMap
       is created as read-only, setting the state back to false could
       lead to unexpected results.  Setting the state to false should
       only be done by the same object that set it to true.

       @param state Read-only state to set this ObjectMap to.  Defaults to true.
     */
    inline void setReadOnly(bool state = true) { read_only_ = state; }


    /**
       Get the name of the variable represented by this ObjectMap.  If
       this ObjectMap has no metadata registered (i.e. it was not
       selected by another ObjectMap), then an empty string will be
       returned, since it has no name.

       @return Name of variable
     */
    std::string getName();


    /**
       Get the full hierarchical name of the variable represented by
       this ObjectMap, based on the path taken to get to this
       object. If this ObjectMap has no metadata registered (i.e. it
       was not selected by another ObjectMap), then an empty string
       will be returned, since it has no name.

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

       @return Address of variable
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
       Increment the reference counter for this ObjectMap. When
       keeping a pointer to the ObjectMap, incRefCount() should be
       called to indicate usage.  When done, call decRefCount() to
       indicate it is no longer needed.
     */
    void incRefCount() { refCount_++; }

    /**
       Decrement the reference counter for this ObjectMap.  If this
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
       Adds a variable to this ObjectMap.  NOTE: calls to this
       function will be ignore if isFundamental() returns true.

       @param name Name of the object in the context of the parent class

       @param obj ObjectMap to add as a variable

     */
    virtual void addVariable(const std::string& UNUSED(name), ObjectMap* UNUSED(obj)) {}


    /************ Functions for getting/setting Object's Value *************/

    /**
       Get the value of the variable as a string.  NOTE: this function
       is only valid for ObjectMaps that represent fundamental types
       or classes treated as fundamental types (i.e. isFundamental()
       returns true).

       @return Value of the represented variable as a string
     */
    virtual std::string get() { return ""; }

    /**
       Sets the value of the variable represented by the ObjectMap to
       the specified value, which is represented as a string.  The
       templated child classes for fundamentals will know how to
       convert the string to a value of the approproprite type.  NOTE:
       this function is only valid for ObjectMaps that represent
       fundamental types or classes treated as fundamentatl types
       (i.e. isFundamental() returns true).

       @param value Value to set the object to, represented as a string

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

       @return Value of the specified variable, represented as a
       string
     */
    virtual std::string get(const std::string& var);

    /**
       Sets the value of the specified variable to the specified
       value, which is represented as a string.  The templated child
       classes for fundamentals will know how to convert the string to
       a value of the approproprite type.  NOTE: this function is only
       valid for ObjectMaps that represent non-fundamental types or
       classes not treated as fundamentatl types (i.e. they must have
       children).

       @param var Name of variable

       @param value Value to set var to represented as a string

       @param[out] found Set to true if var is found, set to false otherwise

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
       Destructor.  NOTE: delete should not be called directly on
       ObjectMaps, rather decRefCount() should be called when the
       object is no longer needed.
     */
    virtual ~ObjectMap() {}

    /**
       Static function to demangle type names returned from typeid(T).name()

       @param name typename returned from typeid(T).name()

       @return demangled name
     */
    static std::string demangle_name(const char* name);

    /**
       Create a string that lists information for the specified
       variable.  This will list all child variables, including the
       values for any children that are fundamentals, up to the
       specified recursion depth.

       @param name Name of variable to list

       @param[out] found Set to true if variable is found, set to false otherwise

       @param recurse Number of levels to recurse (default is 0)

       @return String representing this object and any children
       included based on the value of recurse
     */
    virtual std::string listVariable(std::string name, bool& found, int recurse = 0);

    /**
       Create a string that lists information for the current object.
       This will list all child variables, including the values for
       any children that are fundamentals, up to the specified
       recursion depth.

       @param recurse Number of levels to recurse (default is 0)

       @return String representing this object and any children
       included based on the value of recurse
     */
    virtual std::string list(int recurse = 0);


private:
    /**
       Called to activate this ObjectMap.  This will create the
       metadata object and call activate_callback().

       @param parent ObjectMap parent of this ObjectMap

       @param name Name of this ObjectMap in the context of the parent
     */
    inline void activate(ObjectMap* parent, const std::string& name)
    {
        mdata_ = new ObjectMapMetaData(parent, name);
        activate_callback();
    }

    /**
       Deactivate this object.  This will remove and delete the
       metadata and call deactivate_callback().
     */
    inline void deactivate()
    {
        delete mdata_;
        mdata_ = nullptr;
        deactivate_callback();
    }


    /**
       Find a variable in this object map

       @param name Name of variable to find

       @return ObjectMap representing the requested variable if it is
       found, nullptr otherwise
     */
    inline ObjectMap* findVariable(const std::string& name)
    {
        const std::vector<std::pair<std::string, ObjectMap*>>& variables = getVariables();
        for ( auto& x : variables ) {
            if ( x.first == name ) { return x.second; }
        }
        return nullptr;
    }

    /**
       Function called by list to recurse the object hierarchy

       @param name Name of current ObjectMap

       @param level Current level of recursion

       @param recurse Number of levels deep to recurse
    */
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

    /**
       Default constructor
     */
    ObjectMapWithChildren() : ObjectMap() {}

public:
    /**
       Destructor.  Should not be called directly (i.e. do not call
       delete on the object).  Call decRefCount() when object is no
       longer needed.  This will also call decRefCount() on all its
       children.
     */
    ~ObjectMapWithChildren()
    {
        for ( auto obj : variables_ ) {
            if ( obj.second != nullptr ) obj.second->decRefCount();
        }
        variables_.clear();
    }

    /**
       Adds a variable to this ObjectMap

       @param name Name of the object in the context of this ObjectMap

       @param obj ObjectMap to add as a variable
     */
    void addVariable(const std::string& name, ObjectMap* obj) override
    {
        variables_.push_back(std::make_pair(name, obj));
    }


    /**
       Get the list of child variables contained in this ObjectMap

       @return Reference to vector containing ObjectMaps for this
       ObjectMap's child variables. pair.first is the name of the
       variable in the context of this object. pair.second is a
       pointer to the ObjectMap.
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
    /**
       Default constructor
     */
    ObjectMapHierarchyOnly() : ObjectMapWithChildren() {}


    /**
       Destructor.  Should not be called directly (i.e. do not call
       delete on the object).  Call decRefCount() when object is no
       longer needed.  This will also call decRefCount() on all its
       children.
     */
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
       typeid(T).name() for the type.
     */
    std::string type_;

    /**
       Address of the variable for reading and writing
     */
    void* addr_ = nullptr;

public:
    /**
       Default constructor
     */
    ObjectMapClass() : ObjectMapWithChildren() {}

    /**
       Constructor

       @param addr Address of the object represented by this ObjectMap

       @param type Type of this object as return by typeid(T).name()
     */
    ObjectMapClass(void* addr, const std::string& type) :
        ObjectMapWithChildren(),
        type_(demangle_name(type.c_str())),
        addr_(addr)
    {}

    /**
       Destructor.  Should not be called directly (i.e. do not call
       delete on the object).  Call decRefCount() when object is no
       longer needed.  This will also call decRefCount() on all its
       children.
     */
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
   ObjectMap representing fundamental types, and classes treated as
   fundamental types.  In order for an object to be treated as a
   fundamental, it must be printable using std::to_string() and
   assignable using SST::Core::from_string(). If this is not true, it
   is possible to create a template specialization for the type
   desired to be treated as a fundamental.  The specialization will
   need to provide the functions for getting and setting the values as
   a string.
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
    /**
       Get the value of the object as a string
     */
    virtual std::string get() override { return std::to_string(*addr_); }

    /**
       Set the value of the object represented as a string

       @param value Value to set the underlying object to, represented
       as a string
     */
    virtual void set_impl(const std::string& value) override { *addr_ = SST::Core::from_string<T>(value); }


    /**
       Returns true as object is a fundamental

       @return true
     */
    bool isFundamental() override { return true; }

    /**
       Get the address of the variable represented by the ObjectMap

       @return Address of variable
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

    /**
       Destructor.  Should not be called directly (i.e. do not call
       delete on the object).  Call decRefCount() when object is no
       longer needed.  This will also call decRefCount() on all its
       children.
     */
    ~ObjectMapFundamental() {}

    /**
       Return the type represented by this ObjectMap as given by the
       demangled version of typeid(T).name()

       @return type of underlying object
     */
    std::string getType() override { return demangle_name(typeid(T).name()); }
};


} // namespace Serialization
} // namespace Core
} // namespace SST

#endif // SST_CORE_SERIALIZATION_OBJECTMAP_H
