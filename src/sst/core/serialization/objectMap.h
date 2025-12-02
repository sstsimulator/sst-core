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

#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

// #define _OBJMAP_DEBUG_

namespace SST::Core::Serialization {

// Comparison of two keys: If both keys are integers, use numeric comparison, else lexicographic
struct ObjectMultimapCmp
{
    bool operator()(const std::string& a, const std::string& b) const
    {
        // If the first characters of a and b are digits, attempt to convert them to unsigned long long
        // *ea and *eb are `\0` after strtoull() if there are no invalid characters for integers left in the string
        // errno is nonzero if a value is out of range; we fall back on string comparison then
        if ( isdigit(a[0]) && isdigit(b[0]) ) {
            errno = 0;
            char*              ea;
            unsigned long long na = strtoull(a.c_str(), &ea, 10);
            if ( *ea == '\0' && errno == 0 ) {
                char*              eb;
                unsigned long long nb = strtoull(b.c_str(), &eb, 10);
                if ( *eb == '\0' && errno == 0 ) return na < nb;
            }
        }
        return std::less()(a, b);
    }
};

class ObjectMap;
class TraceBuffer;
class ObjectBuffer;

using ObjectMultimap = std::multimap<std::string, ObjectMap*, ObjectMultimapCmp>;

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
    ObjectMapMetaData(ObjectMap* parent, const std::string& name) :
        parent(parent),
        name(name)
    {}

    ObjectMapMetaData(const ObjectMapMetaData&)            = delete;
    ObjectMapMetaData& operator=(const ObjectMapMetaData&) = delete;
};

/**
   Base class for interacting with data from ObjectMap. This includes
   the ability to keep a history of values and compare the value
   against specified criteria (i.e., value >= 15, value == 6,
   etc). Because this class is type agnostic, interactions will be
   through strings, just as with the ObjectMapClass.

   The implementations of the virtual functions needs to be done in
   templated child clases so that the type of the data is known.
 */
class ObjectMapComparison
{
public:
    enum class Op : std::uint8_t { LT, LTE, GT, GTE, EQ, NEQ, CHANGED, INVALID };

    static Op getOperationFromString(const std::string& op)
    {
        if ( op == "<" ) return Op::LT;
        if ( op == "<=" ) return Op::LTE;
        if ( op == ">" ) return Op::GT;
        if ( op == ">=" ) return Op::GTE;
        if ( op == "==" ) return Op::EQ;
        if ( op == "!=" ) return Op::NEQ;
        if ( op == "changed" ) return Op::CHANGED; // Could also use <>
        return Op::INVALID;
    }

    static std::string getStringFromOp(Op op)
    {
        switch ( op ) {
        case Op::LT:
            return "<";
        case Op::LTE:
            return "<+";
        case Op::GT:
            return ">";
        case Op::GTE:
            return ">=";
        case Op::EQ:
            return "==";
        case Op::NEQ:
            return "!=";
        case Op::CHANGED:
            return "CHANGED";
        case Op::INVALID:
            return "INVALID";
        default:
            return "Invalid Op";
        }
    }

    ObjectMapComparison() = default;

    explicit ObjectMapComparison(const std::string& name) :
        name_(name)
    {}

    ObjectMapComparison(const std::string& name, ObjectMap* obj) :
        name_(name),
        obj_(obj)
    {}

    virtual ~ObjectMapComparison()                        = default;
    virtual bool        compare()                         = 0;
    virtual std::string getCurrentValue() const           = 0;
    virtual void        print(std::ostream& stream) const = 0;
    virtual void*       getVar() const                    = 0;
    const std::string&  getName() const { return name_; }

protected:
    std::string name_ = "";
    ObjectMap*  obj_  = nullptr;
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
    size_t refCount_ = 1;

public:
    /**
       Default constructor primarily used for the "top" object in the hierarchy
     */
    ObjectMap() = default;

    /**
       Check to see if this object is read-only

       @return true if ObjectMap is read-only, false otherwise
     */
    bool isReadOnly() const { return read_only_; }

    /**
       Set the read-only state of the object.  NOTE: If the ObjectMap
       is created as read-only, setting the state back to false could
       lead to unexpected results.  Setting the state to false should
       only be done by the same object that set it to true.

       @param state Read-only state to set this ObjectMap to.  Defaults to true.
     */
    void setReadOnly(bool state = true) { read_only_ = state; }

    /**
     Check if value string is valid for this type

     @param value Value to set the object to expressed as a string
   */
    virtual bool checkValue(const std::string& UNUSED(value)) const { return false; }

    /**
       Get the name of the variable represented by this ObjectMap.  If
       this ObjectMap has no metadata registered (i.e. it was not
       selected by another ObjectMap), then an empty string will be
       returned, since it has no name.

       @return Name of variable
     */
    std::string getName() const { return mdata_ ? mdata_->name : ""; }

    /**
       Get the full hierarchical name of the variable represented by
       this ObjectMap, based on the path taken to get to this
       object. If this ObjectMap has no metadata registered (i.e. it
       was not selected by another ObjectMap), then an empty string
       will be returned, since it has no name.

       @return Full hierarchical name of variable
     */
    std::string getFullName() const;

    /**
       Get the type of the variable represented by the ObjectMap

       @return Type of variable
     */
    virtual std::string getType() const = 0;

    /**
       Get the address of the variable represented by the ObjectMap

       @return Address of variable
     */
    virtual void* getAddr() const = 0;

    /**
       Get the list of child variables contained in this ObjectMap

       @return Reference to map containing ObjectMaps for this
       ObjectMap's child variables. Fundamental types will return the
       same empty map.
     */
    virtual const ObjectMultimap& getVariables() const
    {
        /**
           Static empty variable map for use by versions that don't have
           variables (i.e. are fundamentals or classes treated as
           fundamentals.  This is needed because getVariables() returns a
           reference to the map.
        */
        static ObjectMultimap emptyVars;
        return emptyVars;
    }

    /**
       Increment the reference counter for this ObjectMap. When
       keeping a pointer to the ObjectMap, incRefCount() should be
       called to indicate usage.  When done, call decRefCount() to
       indicate it is no longer needed.
     */
    void incRefCount() { ++refCount_; }

    /**
       Decrement the reference counter for this ObjectMap.  If this
       reference count reaches zero, the object will delete itself.
       NOTE: delete should not be called directly on this object and
       should only be done automatically using decRefCount().
     */
    void decRefCount()
    {
        if ( !--refCount_ ) delete this;
    }

    /**
       Get the current reference count

       @return current value of reference counter for the object
     */
    size_t getRefCount() const { return refCount_; }

    /**
       Get a watch point for this object.  If it is not a valid object
       for a watch point, nullptr will be returned.
    */
    virtual ObjectMapComparison* getComparison(
        const std::string& UNUSED(name), ObjectMapComparison::Op UNUSED(op), const std::string& UNUSED(value)) const
    {
        return nullptr;
    }

    virtual ObjectMapComparison* getComparisonVar(const std::string& UNUSED(name), ObjectMapComparison::Op UNUSED(op),
        const std::string& UNUSED(name2), ObjectMap* UNUSED(var2)) const
    {
        printf("In virtual ObjectMapComparison\n");
        return nullptr;
    }

    virtual ObjectBuffer* getObjectBuffer(const std::string& UNUSED(name), size_t UNUSED(sz)) { return nullptr; }

    /************ Functions for walking the Object Hierarchy ************/

    /**
       Get the parent for this ObjectMap

       @return Parent for this ObjectMap. If this is the top of the
       hierarchy, it will return nullptr
     */
    ObjectMap* selectParent();

    /**
       Get the ObjectMap for the specified variable.
       Important!!! This function return 'this' pointer and not a nullptr!!!
       TODO: prefer this return nullptr as bugs have occurred with incorrect use

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
    virtual std::string get() const { return ""; }

    /**
       Sets the value of the variable represented by the ObjectMap to
       the specified value, which is represented as a string.  The
       templated child classes for fundamentals will know how to
       convert the string to a value of the approproprite type.  NOTE:
       this function is only valid for ObjectMaps that represent
       fundamental types or classes treated as fundamental types
       (i.e. isFundamental() returns true).

       @param value Value to set the object to, represented as a string

    */
    void set(const std::string& value)
    {
        if ( !read_only_ ) set_impl(value);
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
    virtual bool isFundamental() const { return false; }

    /**
       Check to see if this ObjectMap represents a container

       @return true if this ObjectMap represents a container, false
       otherwise
     */
    virtual bool isContainer() const { return false; }

    /**
       Destructor.  NOTE: delete should not be called directly on
       ObjectMaps, rather decRefCount() should be called when the
       object is no longer needed.
     */
    virtual ~ObjectMap() = default;

    /**
       Disallow copying and assignment
     */

    ObjectMap(const ObjectMap&)            = delete;
    ObjectMap& operator=(const ObjectMap&) = delete;

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

    /**
       Find a variable in this object map

       @param name Name of variable to find

       @return ObjectMap representing the requested variable if it is
       found, nullptr otherwise
     */
    ObjectMap* findVariable(const std::string& name) const
    {
        auto& variables = getVariables();
        for ( auto [it, end] = variables.equal_range(name); it != end; ++it )
            return it->second; // For now, we return only the first match if multiple matches
        return nullptr;
    }

private:
    /**
       Called to activate this ObjectMap.  This will create the
       metadata object and call activate_callback().

       @param parent ObjectMap parent of this ObjectMap

       @param name Name of this ObjectMap in the context of the parent
     */
    void activate(ObjectMap* parent, const std::string& name)
    {
        mdata_ = new ObjectMapMetaData(parent, name);
        activate_callback();
    }

    /**
       Deactivate this object.  This will remove and delete the
       metadata and call deactivate_callback().
     */
    void deactivate()
    {
        delete mdata_;
        mdata_ = nullptr;
        deactivate_callback();
    }

    /**
       Function called by list to recurse the object hierarchy

       @param name Name of current ObjectMap

       @param level Current level of recursion

       @param recurse Number of levels deep to recurse
    */
    std::string listRecursive(const std::string& name, int level, int recurse);
}; // class ObjectMap

/**
   ObjectMap object for non-fundamental, non-container types.  This
   class allows for child variables.
 */
class ObjectMapWithChildren : public ObjectMap
{
protected:
    /**
       Map that child ObjectMaps are stored in
     */
    ObjectMultimap variables_;

    /**
       Default constructor
     */
    ObjectMapWithChildren() = default;

public:
    /**
       Destructor.  Should not be called directly (i.e. do not call
       delete on the object).  Call decRefCount() when object is no
       longer needed.  This will also call decRefCount() on all its
       children.
     */
    ~ObjectMapWithChildren() override
    {
        for ( auto& obj : variables_ ) {
            if ( obj.second != nullptr ) {
                obj.second->decRefCount();
            }
        }
    }

    /**
       Disallow copying and assignment
     */

    ObjectMapWithChildren(const ObjectMapWithChildren&)            = delete;
    ObjectMapWithChildren& operator=(const ObjectMapWithChildren&) = delete;

    /**
       Adds a variable to this ObjectMap

       @param name Name of the object in the context of this ObjectMap

       @param obj ObjectMap to add as a variable
     */
    void addVariable(const std::string& name, ObjectMap* obj) final { variables_.emplace(name, obj); }

    /**
       Get the list of child variables contained in this ObjectMap

       @return Reference to map containing ObjectMaps for this ObjectMap's
       child variables. pair.first is the name of the variable in the
       context of this object. pair.second is a pointer to the ObjectMap.
     */
    const ObjectMultimap& getVariables() const final { return variables_; }
}; // class ObjectMapWithChildren

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
    ObjectMapHierarchyOnly() = default;

    /**
       Destructor.  Should not be called directly (i.e. do not call
       delete on the object).  Call decRefCount() when object is no
       longer needed.  This will also call decRefCount() on all its
       children.
     */
    ~ObjectMapHierarchyOnly() override = default;

    /**
       Returns empty string since there is no underlying object being
       represented

       @return empty string
     */
    std::string getType() const override { return ""; }

    /**
       Returns nullptr since there is no underlying object being
       represented

       @return nullptr
     */
    void* getAddr() const override { return nullptr; }
}; // class ObjectMapHierarchyOnly

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
    ObjectMapClass() = default;

    /**
       Disallow copying and assignment
     */

    ObjectMapClass(const ObjectMapClass&)            = delete;
    ObjectMapClass& operator=(const ObjectMapClass&) = delete;

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
    ~ObjectMapClass() override = default;

    /**
       Get the type of the represented object

       @return type of represented object
     */
    std::string getType() const override { return type_; }

    /**
       Get the address of the represented object

       @return address of represented object
     */
    void* getAddr() const override { return addr_; }
}; // class ObjectMapClass

// Whether two types share a common type they can both be converted to
// Users are allowed to provide specializations for std::common_type<T1, T2> for user-defined types
// See https://en.cppreference.com/w/cpp/types/common_type.html
template <class T1, class T2, class = void>
constexpr bool have_common_type_v = false;

template <class T1, class T2>
constexpr bool have_common_type_v<T1, T2, std::void_t<std::common_type_t<T1, T2>>> = true;

// Comparison of two variables if they are convertible to a common type
// See https://en.cppreference.com/w/cpp/language/usual_arithmetic_conversions.html
template <typename T1, typename T2>
std::enable_if_t<have_common_type_v<T1, T2>, bool>
cmp(T1 t1, ObjectMapComparison::Op op, T2 t2)
{
    using T = std::common_type_t<T1, T2>;
    switch ( op ) {
    case ObjectMapComparison::Op::LT:
        return static_cast<T>(t1) < static_cast<T>(t2);
    case ObjectMapComparison::Op::LTE:
        return static_cast<T>(t1) <= static_cast<T>(t2);
    case ObjectMapComparison::Op::GT:
        return static_cast<T>(t1) > static_cast<T>(t2);
    case ObjectMapComparison::Op::GTE:
        return static_cast<T>(t1) >= static_cast<T>(t2);
    case ObjectMapComparison::Op::EQ:
        return static_cast<T>(t1) == static_cast<T>(t2);
    case ObjectMapComparison::Op::NEQ:
    case ObjectMapComparison::Op::CHANGED:
        return static_cast<T>(t1) != static_cast<T>(t2);
    default:
        std::cout << "Invalid comparison operator\n";
        return false;
    }
}

// Comparison of two variables if they are not convertible to a common type
template <typename T1, typename T2>
std::enable_if_t<!have_common_type_v<T1, T2>, bool>
cmp(T1 UNUSED(t1), ObjectMapComparison::Op UNUSED(op), T2 UNUSED(t2))
{
    // We shouldn't get here.... Can I throw an error somehow?
    printf("ERROR: cmp() does not support comparison of two types without a std::common_type\n");
    return false;
}

/**
   Template implementation of ObjectMapComparison for <var> <op> <value>
 */
template <typename T, typename REF = T>
class ObjectMapComparison_impl : public ObjectMapComparison
{
public:
    ObjectMapComparison_impl(const std::string& name, REF* var, Op op, const std::string& value) :
        ObjectMapComparison(name),
        var_(var),
        op_(op)
    {
        if ( op_ == Op::CHANGED ) {
            // If we are looking for changes, get the current value as the comp_value_
            comp_value_ = static_cast<T>(*var_);
        }
        else {
            // Otherwise, we have to have a valid value.  If the value is not valid, it will throw an exception.
            comp_value_ = SST::Core::from_string<T>(value);
        }
    }

    bool compare() override
    {
        // Get the result of the comparison
        bool ret = cmp(*var_, op_, comp_value_);

        // For change detection, store the current value for the next test
        if ( op_ == Op::CHANGED ) comp_value_ = static_cast<T>(*var_);

        return ret;
    }

    std::string getCurrentValue() const override { return SST::Core::to_string(*var_); }

    void* getVar() const override { return var_; }

    void print(std::ostream& stream) const override
    {
        stream << name_ << " " << getStringFromOp(op_);
        if ( op_ == Op::CHANGED )
            stream << " ";
        else
            stream << " " << SST::Core::to_string(comp_value_) << " ";
    }

private:
    REF* const var_;
    Op const   op_;
    T          comp_value_;
}; // class ObjectMapComparison_impl

/**
Template implementation of ObjectMapComparison for <var> <op> <var>
*/
template <typename T1, typename T2>
class ObjectMapComparison_var : public ObjectMapComparison
{
public:
    ObjectMapComparison_var(const std::string& name1, T1* var1, Op op, const std::string& name2, T2* var2) :
        ObjectMapComparison(name1),
        name2_(name2),
        var1_(var1),
        op_(op),
        var2_(var2)
    {}

    bool compare() override
    {
        T1 v1 = *var1_;
        T2 v2 = *var2_;
        return cmp(v1, op_, v2);
    }

    std::string getCurrentValue() const override
    {
        return SST::Core::to_string(*var1_) + " " + SST::Core::to_string(*var2_);
    }

    void* getVar() const override { return var1_; }

    void print(std::ostream& stream) const override
    {
        stream << name_ << " " << getStringFromOp(op_);
        if ( op_ == Op::CHANGED )
            stream << " ";
        else
            stream << " " << name2_ << " ";
    }

private:
    std::string const name2_;
    T1* const         var1_;
    Op const          op_;
    T2* const         var2_;
}; // class ObjectMapComparison_impl

class ObjectBuffer
{
public:
    ObjectBuffer(const std::string& name, size_t sz) :
        name_(name),
        bufSize_(sz)
    {}

    virtual ~ObjectBuffer() = default;

    virtual void        sample(size_t index, bool trigger) = 0;
    virtual std::string get(size_t index) const            = 0;
    virtual std::string getTriggerVal() const              = 0;

    std::string getName() const { return name_; }
    size_t      getBufSize() const { return bufSize_; }

private:
    std::string name_;
    size_t      bufSize_;
}; // class ObjectBuffer

template <typename T, typename REF = T>
class ObjectBuffer_impl : public ObjectBuffer
{
public:
    ObjectBuffer_impl(const std::string& name, REF* varPtr, size_t sz) :
        ObjectBuffer(name, sz),
        varPtr_(varPtr)
    {
        objectBuffer_.resize(sz);
    }

    void sample(size_t index, bool trigger) override
    {
        objectBuffer_[index] = *varPtr_;
        if ( trigger ) triggerVal = *varPtr_;
    }

    std::string get(size_t index) const override { return SST::Core::to_string(objectBuffer_.at(index)); }

    std::string getTriggerVal() const override { return SST::Core::to_string(triggerVal); }

private:
    REF* const     varPtr_;
    std::vector<T> objectBuffer_;
    T              triggerVal {};
}; // class ObjectBuffer_impl

class TraceBuffer
{

public:
    TraceBuffer(Core::Serialization::ObjectMap* var, size_t sz, size_t pdelay) :
        varObj_(var),
        bufSize_(sz),
        postDelay_(pdelay)
    {
        tagBuffer_.resize(bufSize_);
        cycleBuffer_.resize(bufSize_);
        handlerBuffer_.resize(bufSize_);
    }

    virtual ~TraceBuffer() = default;

    void setBufferReset() { reset_ = true; }

    void resetTraceBuffer()
    {
        printf("    Reset Trace Buffer\n");
        postCount_   = 0;
        cur_         = 0;
        first_       = 0;
        numRecs_     = 0;
        samplesLost_ = 0;
        isOverrun_   = false;
        reset_       = false;
        state_       = CLEAR;
    }

    size_t getBufferSize() { return bufSize_; }

    void addObjectBuffer(ObjectBuffer* vb)
    {
        objBuffers_.push_back(vb);
        numObjects++;
    }

    enum BufferState : int {
        CLEAR,       // 0 Pre Trigger
        TRIGGER,     // 1 Trigger
        POSTTRIGGER, // 2 Post Trigger
        OVERRUN      // 3 Overrun
    };

    const std::map<BufferState, char> state2char { { CLEAR, '-' }, { TRIGGER, '!' }, { POSTTRIGGER, '+' },
        { OVERRUN, 'o' } };

    bool sampleT(bool trigger, uint64_t cycle, const std::string& handler)
    {
        size_t start_state  = state_;
        bool   invokeAction = false;

        // if Trigger == TRUE
        if ( trigger ) {
            if ( start_state == CLEAR ) { // Not previously triggered
                state_ = TRIGGER;         // State becomes trigger record
            }
            // printf("    Sample: trigger\n");

        } // if trigger

        if ( start_state == TRIGGER || start_state == POSTTRIGGER ) { // trigger record or post trigger
            state_ = POSTTRIGGER;                                     // State becomes post trigger
            // printf("    Sample: post trigger\n");
        }

// Circular buffer
#ifdef _OBJMAP_DEBUG_
        std::cout << "    Sample:" << handler << ": numRecs:" << numRecs_ << " first:" << first_ << " cur:" << cur_
                  << " state:" << state2char.at(state_) << " isOverrun:" << isOverrun_
                  << " samplesLost:" << samplesLost_ << std::endl;
#endif
        cycleBuffer_[cur_]   = cycle;
        handlerBuffer_[cur_] = handler;
        if ( trigger ) {
            triggerCycle = cycle;
        }

        // Sample all the trace object buffers
        ObjectBuffer* varBuffer_;
        for ( size_t obj = 0; obj < numObjects; obj++ ) {
            varBuffer_ = objBuffers_[obj];
            varBuffer_->sample(cur_, trigger);
        }

        if ( numRecs_ < bufSize_ ) {
            tagBuffer_[cur_] = state_;
            numRecs_++;
            cur_ = (cur_ + 1) % bufSize_;
            if ( cur_ == 0 ) first_ = 0; // 1;
        }
        else { // Buffer full
            // Check to see if we are overwriting trigger
            if ( tagBuffer_[cur_] == TRIGGER ) {
                // printf("    Sample Overrun\n");
                isOverrun_ = true;
            }
            tagBuffer_[cur_] = state_;
            numRecs_++;
            cur_   = (cur_ + 1) % bufSize_;
            first_ = cur_;
        }

        if ( isOverrun_ ) {
            samplesLost_++;
        }

        if ( (state_ == TRIGGER) && (postDelay_ == 0) ) {
            invokeAction = true;
            std::cout << "    Invoke Action\n";
        }

        if ( state_ == POSTTRIGGER ) {
            postCount_++;
            if ( postCount_ >= postDelay_ ) {
                invokeAction = true;
                std::cout << "    Invoke Action\n";
            }
        }

        return invokeAction;
    }

    void dumpTraceBufferT()
    {
        if ( numRecs_ == 0 ) return;

        size_t start;
        size_t end;

        start = first_;
        if ( cur_ == 0 ) {
            end = bufSize_ - 1;
        }
        else {
            end = cur_ - 1;
        }
        // std::cout << "start=" << start << " end=" << end << std::endl;

        for ( int j = start;; j++ ) {
            size_t i = j % bufSize_;

            std::cout << "buf[" << i << "] " << handlerBuffer_.at(i) << " @" << cycleBuffer_.at(i) << " ("
                      << state2char.at(tagBuffer_.at(i)) << ") ";

            for ( size_t obj = 0; obj < numObjects; obj++ ) {
                ObjectBuffer* varBuffer_ = objBuffers_[obj];
                std::cout << SST::Core::to_string(varBuffer_->getName()) << "=" << varBuffer_->get(i) << " ";
            }
            std::cout << std::endl;

            if ( i == end ) {
                break;
            }
        }
    }

    void dumpTriggerRecord()
    {
        if ( numRecs_ == 0 ) {
            std::cout << "No trace samples in current buffer" << std::endl;
            return;
        }
        if ( state_ != CLEAR ) {
            std::cout << "TriggerRecord:@cycle" << triggerCycle << ": samples lost = " << samplesLost_ << ": ";
            for ( size_t obj = 0; obj < numObjects; obj++ ) {
                ObjectBuffer* varBuffer_ = objBuffers_[obj];
                std::cout << SST::Core::to_string(varBuffer_->getName()) << "=" << varBuffer_->getTriggerVal() << " ";
            }
            std::cout << std::endl;
        }
    }

    void printVars()
    {
        for ( size_t obj = 0; obj < numObjects; obj++ ) {
            ObjectBuffer* varBuffer_ = objBuffers_[obj];
            std::cout << SST::Core::to_string(varBuffer_->getName()) << " ";
        }
    }

    void printConfig()
    {
        std::cout << "bufsize = " << bufSize_ << " postDelay = " << postDelay_ << " : ";
        printVars();
    }

    // private:
    Core::Serialization::ObjectMap* varObj_      = nullptr;
    size_t                          bufSize_     = 64;
    size_t                          postDelay_   = 8;
    size_t                          postCount_   = 0;
    size_t                          cur_         = 0;
    size_t                          first_       = 0;
    size_t                          numRecs_     = 0;
    bool                            isOverrun_   = false;
    size_t                          samplesLost_ = 0;
    bool                            reset_       = false;
    BufferState                     state_       = CLEAR;

    size_t                     numObjects = 0;
    std::vector<BufferState>   tagBuffer_;
    std::vector<std::string>   handlerBuffer_;
    std::vector<ObjectBuffer*> objBuffers_;
    std::vector<uint64_t>      cycleBuffer_;
    uint64_t                   triggerCycle;

}; // class TraceBuffer

/**
   ObjectMap representing fundamental types, and classes treated as
   fundamental types.  In order for an object to be treated as a
   fundamental, it must be printable using SST::Core::to_string() and
   assignable using SST::Core::from_string(). If this is not true, it
   is possible to create a template specialization for the type
   desired to be treated as a fundamental.  The specialization will
   need to provide the functions for getting and setting the values as
   a string.

   REF defaults to T, but may be a type like std::vector<bool>::reference
   or std::bitset<N>::reference indicating a proxy reference to the object.
*/
template <typename T, typename REF = T>
class ObjectMapFundamental : public ObjectMap
{
protected:
    /**
       Address of the variable for reading and writing
     */
    REF* addr_ = nullptr;

public:
    /**
       Set the value of the object represented as a string

       @param value Value to set the underlying object to, represented
       as a string
     */
    virtual void set_impl(const std::string& value) override
    {
        try {
            *addr_ = SST::Core::from_string<T>(value);
        }
        catch ( const std::exception& e ) {
            std::cerr << e.what() << ": " << value << std::endl;
        }
    }

    bool checkValue(const std::string& value) const override
    {
        try {
            *addr_ = SST::Core::from_string<T>(value);
            return true;
        }
        catch ( const std::exception& e ) {
            std::cerr << e.what() << ": " << value << std::endl;
            return false;
        }
    }

    /**
       Get the value of the object as a string
     */
    std::string get() const override { return addr_ ? SST::Core::to_string(static_cast<T>(*addr_)) : "nullptr"; }

    /**
       Returns true as object is a fundamental

       @return true
     */
    bool isFundamental() const final { return true; }

    /**
       Get the address of the variable represented by the ObjectMap

       @return Address of variable
     */
    void* getAddr() const override { return addr_; }

    explicit ObjectMapFundamental(REF* addr) :
        addr_(addr)
    {}

    /**
       Destructor.  Should not be called directly (i.e. do not call
       delete on the object).  Call decRefCount() when object is no
       longer needed.  This will also call decRefCount() on all its
       children.
     */
    ~ObjectMapFundamental() override = default;

    /**
       Disallow copying and assignment
     */

    ObjectMapFundamental(const ObjectMapFundamental&)            = delete;
    ObjectMapFundamental& operator=(const ObjectMapFundamental&) = delete;

    /**
       Return the type represented by this ObjectMap as given by the
       demangled version of typeid(T).name()

       @return type of underlying object
     */
    std::string getType() const override { return demangle_name(typeid(T).name()); }

    ObjectMapComparison* getComparison(
        const std::string& name, ObjectMapComparison::Op op, const std::string& value) const override
    {
        return new ObjectMapComparison_impl<T, REF>(name, addr_, op, value);
    }

    ObjectMapComparison* getComparisonVar(
        const std::string& name, ObjectMapComparison::Op op, const std::string& name2, ObjectMap* var2) const override
    {
        // Ensure var2 is fundamental type
        if ( !var2->isFundamental() ) {
            printf("Triggers can only use fundamental types; %s is not fundamental\n", name2.c_str());
            return nullptr;
        }

        std::string type = var2->getType();
#if 0
        std::cout << "In ObjectMapComparison_var: " << name << " " << name2 << std::endl;
        // std::cout << "typeid(T): " << demangle_name(typeid(T).name()) << std::endl;
        std::string type1 = getType();
        std::cout << "getType(v1): " << type1 << std::endl;
        std::cout << "getType(v2): " << type << std::endl;
#endif

        // Create ObjectMapComparison_var which compares two variables
        // Only support arithmetic types for now
        if constexpr ( std::is_arithmetic_v<T> ) {
            if ( type == "int" ) {
                return new ObjectMapComparison_var<REF, int>(
                    name, addr_, op, name2, static_cast<int*>(var2->getAddr()));
            }
            else if ( type == "unsigned" || type == "unsigned int" ) {
                return new ObjectMapComparison_var<REF, unsigned>(
                    name, addr_, op, name2, static_cast<unsigned*>(var2->getAddr()));
            }
            else if ( type == "long" ) {
                return new ObjectMapComparison_var<REF, long>(
                    name, addr_, op, name2, static_cast<long*>(var2->getAddr()));
            }
            else if ( type == "unsigned long" ) {
                return new ObjectMapComparison_var<REF, unsigned long>(
                    name, addr_, op, name2, static_cast<unsigned long*>(var2->getAddr()));
            }
            else if ( type == "char" ) {
                return new ObjectMapComparison_var<REF, char>(
                    name, addr_, op, name2, static_cast<char*>(var2->getAddr()));
            }
            else if ( type == "signed char" ) {
                return new ObjectMapComparison_var<REF, signed char>(
                    name, addr_, op, name2, static_cast<signed char*>(var2->getAddr()));
            }
            else if ( type == "unsigned char" ) {
                return new ObjectMapComparison_var<REF, unsigned char>(
                    name, addr_, op, name2, static_cast<unsigned char*>(var2->getAddr()));
            }
            else if ( type == "short" ) {
                return new ObjectMapComparison_var<REF, short>(
                    name, addr_, op, name2, static_cast<short*>(var2->getAddr()));
            }
            else if ( type == "unsigned short" ) {
                return new ObjectMapComparison_var<REF, unsigned short>(
                    name, addr_, op, name2, static_cast<unsigned short*>(var2->getAddr()));
            }
            else if ( type == "long long" ) {
                return new ObjectMapComparison_var<REF, long long>(
                    name, addr_, op, name2, static_cast<long long*>(var2->getAddr()));
            }
            else if ( type == "unsigned long long" ) {
                return new ObjectMapComparison_var<REF, unsigned long long>(
                    name, addr_, op, name2, static_cast<unsigned long long*>(var2->getAddr()));
            }
            else if ( type == "bool" ) {
                return new ObjectMapComparison_var<REF, bool>(
                    name, addr_, op, name2, static_cast<bool*>(var2->getAddr()));
            }
            else if ( type == "float" ) {
                return new ObjectMapComparison_var<REF, float>(
                    name, addr_, op, name2, static_cast<float*>(var2->getAddr()));
            }
            else if ( type == "double" ) {
                return new ObjectMapComparison_var<REF, double>(
                    name, addr_, op, name2, static_cast<double*>(var2->getAddr()));
            }
            else if ( type == "long double" ) {
                return new ObjectMapComparison_var<REF, long double>(
                    name, addr_, op, name2, static_cast<long double*>(var2->getAddr()));
            }
        } // end if first var is arithmetic

        std::cout << "Invalid type for comparison: " << name2 << "(" << type << ")\n";
        return nullptr;
    }

    ObjectBuffer* getObjectBuffer(const std::string& name, size_t sz) override
    {
        return new ObjectBuffer_impl<T, REF>(name, addr_, sz);
    }
};

/**
   Class used to map containers
 */
template <class T>
class ObjectMapContainer : public ObjectMapWithChildren
{
protected:
    T* addr_;

public:
    explicit ObjectMapContainer(T* addr) :
        addr_(addr)
    {}
    bool        isContainer() const final { return true; }
    std::string getType() const override { return demangle_name(typeid(T).name()); }
    void*       getAddr() const override { return addr_; }
    ~ObjectMapContainer() override = default;
};

/**
   Class used to map arrays
 */
template <class T>
class ObjectMapArray : public ObjectMapContainer<T>
{
protected:
    size_t size;

public:
    virtual size_t getSize() const { return size; }
    ObjectMapArray(T* addr, size_t size) :
        ObjectMapContainer<T>(addr),
        size(size)
    {}
    ~ObjectMapArray() override = default;
};

// ObjectMap for reference proxy types such as std::bitset<N>::reference, std::vector<bool>::reference,
// atomic_reference, whose referenced types cannot be copied or pointed to with pointers, but whose
// underlying values are ordinary fundamental types.
//
//     T is the underlying fundamental type, such as bool, which is the type in which values are read and written.
//   REF is the type of the proxy reference class, which is copyable, convertible to T, and assignable from T.
// PTYPE is the type to print, which defaults to T, but might be a decorated class name like std::atomic<T>.
template <typename T, typename REF, typename PTYPE = T>
class ObjectMapFundamentalReference : public ObjectMapFundamental<T, REF>
{
    REF ref;

    static_assert(std::is_copy_constructible_v<REF> && std::is_convertible_v<REF, T> && std::is_assignable_v<REF, T>,
        "ObjectMapFundamentalReference<T, REF, PTYPE>: REF must be copyable, implicitly convertible to T, and "
        "assignable from T.");

public:
    // std::addressof is used because some libraries overload REF::operator&()
    explicit ObjectMapFundamentalReference(const REF& ref) :
        ObjectMapFundamental<T, REF>(std::addressof(this->ref)),
        ref(ref)
    {}

    // Although this is a fundamental type of underlying type T, PTYPE can be something like std::atomic<T>
    std::string getType() const override { return this->demangle_name(typeid(PTYPE).name()); }

    ObjectMapFundamentalReference(const ObjectMapFundamentalReference&)            = default;
    ObjectMapFundamentalReference& operator=(const ObjectMapFundamentalReference&) = delete;
    ~ObjectMapFundamentalReference() override                                      = default;
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_OBJECTMAP_H
