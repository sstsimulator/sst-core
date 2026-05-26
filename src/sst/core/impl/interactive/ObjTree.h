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

#ifndef SST_CORE_SERIALIZATION_OBJECTTREE_DEBUGGER_H
#define SST_CORE_SERIALIZATION_OBJECTTREE_DEBUGGER_H

#include "sst/core/baseComponent.h"
#include "sst/core/componentInfo.h"
#include "sst/core/from_string.h"
#include "sst/core/serialization/objectMapDeferred.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <any>
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
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace SST::Core::Serialization {

class ObjTreeCont
{
public:
    enum class NodeKind { Generic, Integer, Float, String, Bool, Component, Container, GenericVal };

    ObjTreeCont() :
        parent_(nullptr),
        children_(),
        name_("uninit"),
        type_("uninit"),
        kind_(NodeKind::Generic),
        readOnly_(false) {};
    ObjTreeCont(const std::string& name, const std::string& type, NodeKind kind = NodeKind::Generic) :
        parent_(nullptr),
        children_(),
        name_(name),
        type_(type),
        kind_(kind),
        readOnly_(false)
    {}
    virtual ~ObjTreeCont() = default;

    ObjTreeCont(const ObjTreeCont& rhs) :
        parent_(nullptr),
        children_(),
        name_(rhs.name_),
        type_(rhs.type_),
        kind_(rhs.kind_),
        readOnly_(rhs.readOnly_)
    {
        children_.reserve(rhs.children_.size());
        for ( const auto& child : rhs.children_ ) {
            auto* cloned    = child->clone();
            cloned->parent_ = this;
            children_.emplace_back(cloned);
        }
    }

    ObjTreeCont& operator=(const ObjTreeCont& rhs)
    {
        if ( this == &rhs ) return *this;
        parent_ = nullptr;
        name_   = rhs.name_;
        type_   = rhs.type_;
        kind_   = rhs.kind_;
        children_.clear();
        children_.reserve(rhs.children_.size());
        for ( const auto& child : rhs.children_ ) {
            auto* cloned    = child->clone();
            cloned->parent_ = this;
            children_.emplace_back(cloned);
        }
        return *this;
    }

    virtual ObjTreeCont* clone() const { return new ObjTreeCont(*this); }

    void addChildObj(ObjTreeCont* obj)
    {
        children_.push_back(std::unique_ptr<ObjTreeCont>(obj));
        obj->parent_ = this;
    }

    void                                             setParent(ObjTreeCont* p) { parent_ = p; }
    ObjTreeCont*                                     getParent() const { return parent_; }
    const std::vector<std::unique_ptr<ObjTreeCont>>& getChildren() const { return children_; }

    const std::string& getObjName() const { return name_; }
    const std::string& getType() const { return type_; }
    void               setName(const std::string& name) { name_ = name; }
    void               setType(const std::string& type) { type_ = prettifyType(type); }
    bool               isRoot() { return parent_ == nullptr; }
    void               makeReadOnly() { readOnly_ = true; }
    bool               isReadOnly() { return readOnly_; }

    template <typename Func>
    void applyRecursive(Func&& func)
    {
        for ( auto& child : children_ ) {
            func(child.get());
        }
    }

    template <typename ObjectType, typename Func>
    void applyRecursiveByType(Func&& func)
    {
        for ( auto& child : children_ ) {
            if ( auto* child_t = dynamic_cast<ObjectType*>(child.get()) ) {
                func(child_t);
            }
        }
    }

    template <typename ObjectType, typename Func>
    ObjectType* findByType(Func&& predicate)
    {
        for ( auto& child : children_ ) {
            if ( auto* child_t = dynamic_cast<ObjectType*>(child.get()) ) {
                if ( predicate(child_t) ) return child_t;
            }
        }
        return nullptr;
    }

    ObjTreeCont* findByName(const std::string name)
    {
        for ( size_t i = 0; i < children_.size(); i++ ) {
            if ( children_[i]->getObjName() == name ) {
                return children_[i].get();
            }
        }
        return nullptr;
    }

    virtual void        apply() {};
    virtual std::string getTypeName() const { return type_; };
    virtual void        Dump(
               const int verbosity, std::ios_base::fmtflags base = std::ios_base::dec, std::ostream& os = std::cout)
    {
        os << name_ << "/ " << std::endl; //(" << type_ << ")" << std::endl;
        if ( verbosity > 0 ) {
            applyRecursive([&](ObjTreeCont* child) { child->Dump(verbosity - 1, base, os); });
        }
    }

    virtual bool setFromString([[maybe_unused]] const std::string& value) { return false; }
    virtual void syncFromSim() {};
    virtual bool hasChanged() { return false; }

    NodeKind getKind() const { return kind_; }
    bool     isComponent() const { return kind_ == NodeKind::Component; }
    bool     isLeaf() const
    {
        return kind_ == NodeKind::Integer || kind_ == NodeKind::Float || kind_ == NodeKind::String ||
               kind_ == NodeKind::Bool || kind_ == NodeKind::GenericVal;
    }

    virtual void clear() { children_.clear(); }

protected:
    ObjTreeCont*                              parent_;
    std::vector<std::unique_ptr<ObjTreeCont>> children_;
    std::string                               name_;
    std::string                               type_;
    NodeKind                                  kind_;
    bool                                      readOnly_;

    static void replaceAll(std::string& s, std::string_view from, std::string_view to)
    {
        if ( from.empty() ) return;
        size_t pos = 0;
        while ( (pos = s.find(from, pos)) != std::string::npos ) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    static std::string prettifyType(std::string t)
    {
        replaceAll(t, "std::__1::", "std::");
        replaceAll(t, "std::__cxx11::", "std::");
        replaceAll(t, "  ", " ");
        return t;
    }
};

template <typename Obj_T>
class ObjTree : public ObjTreeCont
{
public:
    ObjTree() = default;
    ObjTree(NodeKind kind) :
        ObjTreeCont("", "", kind)
    {}

    ObjTree(const ObjTree<Obj_T>& rhs) :
        ObjTreeCont(rhs),
        objects_()
    {
        objects_.reserve(rhs.objects_.size());
        for ( const auto& obj : rhs.objects_ ) {
            auto* cloned = obj->clone();
            cloned->setParent(this);
            objects_.emplace_back(cloned);
        }
    }

    ObjTree<Obj_T>& operator=(const ObjTree<Obj_T>& other)
    {
        if ( this == &other ) return *this;
        ObjTreeCont::operator=(other);
        objects_.clear();
        objects_.reserve(other.objects_.size());
        for ( const auto& obj : other.objects_ ) {
            auto* cloned = obj->clone();
            cloned->setParent(this);
            objects_.emplace_back(cloned);
        }
        return *this;
    }

    void BuildTree(const ComponentInfoMap& compMap);

    Obj_T&       getObj() { return static_cast<Obj_T&>(*this); }
    const Obj_T& getObj() const { return static_cast<const Obj_T&>(*this); }


    template <typename ObjectType, typename Func>
    void applyRecursiveByType(Func&& func)
    {
        for ( auto& child : children_ ) {
            if ( auto* child_t = dynamic_cast<ObjectType*>(child.get()) ) {
                func(child_t);
            }
        }
    }

    std::string getTypeName() const override { return typeid(Obj_T).name(); }

    bool isEmpty() { return objects_.empty() && children_.empty(); }

    void Dump([[maybe_unused]] const int verbosity, [[maybe_unused]] std::ios_base::fmtflags base = std::ios_base::dec,
        std::ostream& os = std::cout) override
    {
        os << "Root/" << std::endl;
    }
    void clear() override
    {
        ObjTreeCont::clear();
        objects_.clear();
    }

protected:
    std::vector<std::unique_ptr<ObjTreeCont>> objects_;
};

class IntegerObj : public ObjTree<IntegerObj>
{
public:
    using IntVariant = std::variant<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

private:
    IntVariant val_;
    void*      addr_;

public:
    template <typename T>
    IntegerObj(T v, void* addr) :
        ObjTree<IntegerObj>(NodeKind::Integer),
        val_(v),
        addr_(addr)
    {}

    IntegerObj(const IntegerObj& rhs) :
        ObjTree<IntegerObj>(rhs),
        val_(rhs.val_),
        addr_(rhs.addr_)
    {}
    IntegerObj& operator=(const IntegerObj& rhs)
    {
        if ( this == &rhs ) return *this;
        ObjTree<IntegerObj>::operator=(rhs);
        val_  = rhs.val_;
        addr_ = rhs.addr_;
        return *this;
    }
    ObjTreeCont* clone() const override { return new IntegerObj(*this); }

    template <typename T>
    T getVal() const
    {
        return std::get<T>(val_);
    }

    template <typename T>
    void setVal(T v)
    {
        val_ = v;
    }

    template <typename T>
    void setSimVal(T v)
    {
        val_ = v;
        syncToSim();
    }

    virtual void syncFromSim() override
    {
        if ( !addr_ ) return;
        std::visit([this](auto& v) { v = *static_cast<std::decay_t<decltype(v)>*>(addr_); }, val_);
    }

    virtual bool hasChanged() override
    {
        if ( !addr_ ) return false;
        return std::visit(
            [this](const auto& v) -> bool { return v != *static_cast<std::decay_t<decltype(v)>*>(addr_); }, val_);
    }

    template <typename Visitor>
    auto visit(Visitor&& visitor)
    {
        return std::visit(std::forward<Visitor>(visitor), val_);
    }

    template <typename Visitor>
    auto visit(Visitor&& visitor) const
    {
        return std::visit(std::forward<Visitor>(visitor), val_);
    }

    void apply() override
    {
        visit([](auto val) { std::cout << "Processing integer: " << static_cast<int64_t>(val) << std::endl; });
    }

    void Dump(
        const int verbosity, std::ios_base::fmtflags base = std::ios_base::dec, std::ostream& os = std::cout) override
    {
        std::string ro = (readOnly_) ? " (ro)" : "";
        if ( verbosity == 0 ) {
            os << getObjName() << ro << std::endl;
        }
        else if ( verbosity == 1 ) {
            visit([&](auto val) {
                auto old_flags = os.flags();
                os.setf(base, std::ios_base::basefield);
                if ( base != std::ios_base::dec ) {
                    os.setf(std::ios_base::showbase);
                }
                os << getObjName() << ro << " = " << static_cast<int64_t>(val) << std::endl;
                os.flags(old_flags);
            });
        }
        else {
            visit([&](auto val) {
                auto old_flags = os.flags();
                os.setf(base, std::ios_base::basefield);
                if ( base != std::ios_base::dec ) {
                    os.setf(std::ios_base::showbase);
                }
                os << getObjName() << ro << " = " << static_cast<int64_t>(val) << " (" << getType() << ")" << std::endl;
                os.flags(old_flags);
            });
        }
    }
    bool setFromString(const std::string& value) override
    {
        if ( readOnly_ ) {
            return false;
        }
        try {
            visit([&](auto& current) {
                using T = std::decay_t<decltype(current)>;
                setSimVal(static_cast<T>(std::stoll(value)));
            });
            return true;
        }
        catch ( ... ) {
            return false;
        }
    }

private:
    void syncToSim()
    {
        if ( !addr_ ) return;
        std::visit([this](auto& v) { *static_cast<std::decay_t<decltype(v)>*>(addr_) = v; }, val_);
    }
};

class FloatObj : public ObjTree<FloatObj>
{
public:
    using FloatVariant = std::variant<float, double, long double>;

private:
    FloatVariant val_;
    void*        addr_;

public:
    template <typename T>
    FloatObj(T v, void* addr) :
        ObjTree<FloatObj>(NodeKind::Float),
        val_(v),
        addr_(addr)
    {}

    FloatObj(const FloatObj& rhs) :
        ObjTree<FloatObj>(rhs),
        val_(rhs.val_),
        addr_(rhs.addr_)
    {}

    FloatObj& operator=(const FloatObj& rhs)
    {
        if ( this == &rhs ) return *this;
        ObjTree<FloatObj>::operator=(rhs);
        val_  = rhs.val_;
        addr_ = rhs.addr_;
        return *this;
    }

    ObjTreeCont* clone() const override { return new FloatObj(*this); }

    template <typename T>
    T getVal() const
    {
        return std::get<T>(val_);
    }

    template <typename T>
    void setVal(T v)
    {
        val_ = v;
    }

    template <typename T>
    void setSimVal(T v)
    {
        val_ = v;
        syncToSim();
    }

    virtual void syncFromSim() override
    {
        if ( !addr_ ) return;
        std::visit([this](auto& v) { v = *static_cast<std::decay_t<decltype(v)>*>(addr_); }, val_);
    }

    virtual bool hasChanged() override
    {
        if ( !addr_ ) return false;
        return std::visit(
            [this](const auto& v) -> bool { return v != *static_cast<std::decay_t<decltype(v)>*>(addr_); }, val_);
    }

    template <typename Visitor>
    auto visit(Visitor&& visitor)
    {
        return std::visit(std::forward<Visitor>(visitor), val_);
    }

    template <typename Visitor>
    auto visit(Visitor&& visitor) const
    {
        return std::visit(std::forward<Visitor>(visitor), val_);
    }

    void apply() override
    {
        visit([](auto val) { std::cout << "Processing float: " << val << std::endl; });
    }
    void Dump(
        const int verbosity, std::ios_base::fmtflags base = std::ios_base::dec, std::ostream& os = std::cout) override
    {
        std::string ro = (readOnly_) ? " (ro)" : "";
        if ( verbosity == 0 ) {
            os << getObjName() << ro << std::endl;
        }
        else if ( verbosity == 1 ) {
            visit([&](auto val) {
                auto old_flags = os.flags();
                auto old_prec  = os.precision();
                os.setf(base, std::ios_base::basefield);
                if ( base != std::ios_base::dec ) {
                    os.setf(std::ios_base::showbase);
                }
                os << getObjName() << ro << std::fixed << std::setprecision(12) << " = " << val << std::endl;
                os.flags(old_flags);
                os.precision(old_prec);
            });
        }
        else {
            visit([&](auto val) {
                auto old_flags = os.flags();
                auto old_prec  = os.precision();
                os.setf(base, std::ios_base::basefield);
                if ( base != std::ios_base::dec ) {
                    os.setf(std::ios_base::showbase);
                }
                os << getObjName() << ro << std::fixed << std::setprecision(12) << " = " << val << " (" << getType()
                   << ")" << std::endl;
                os.flags(old_flags);
                os.precision(old_prec);
            });
        }
    }

    bool setFromString(const std::string& value) override
    {
        if ( readOnly_ ) {
            return false;
        }
        try {
            visit([&](auto& current) {
                using T = std::decay_t<decltype(current)>;
                setSimVal(static_cast<T>(std::stold(value)));
            });
            return true;
        }
        catch ( ... ) {
            return false;
        }
    }

private:
    void syncToSim()
    {
        if ( !addr_ ) return;
        std::visit([this](auto& v) { *static_cast<std::decay_t<decltype(v)>*>(addr_) = v; }, val_);
    }
};

class ComponentObj : public ObjTree<ComponentObj>
{

private:
    BaseComponent* val_      = nullptr;
    ComponentInfo* compInfo_ = nullptr;

public:
    ComponentObj() = default;
    ComponentObj(BaseComponent* v, ComponentInfo* ci) :
        ObjTree<ComponentObj>(NodeKind::Component),
        val_(v),
        compInfo_(ci)
    {
        setName(v->getName());
    }

    ComponentObj(const ComponentObj& rhs) :
        ObjTree<ComponentObj>(rhs),
        val_(rhs.val_),
        compInfo_(rhs.compInfo_)
    {}

    ComponentObj& operator=(const ComponentObj& rhs)
    {
        if ( this == &rhs ) return *this;
        ObjTree<ComponentObj>::operator=(rhs);
        val_      = rhs.val_;
        compInfo_ = rhs.compInfo_;
        return *this;
    }

    ObjTreeCont* clone() const override { return new ComponentObj(*this); }

    BaseComponent* getVal() const { return val_; }
    ComponentInfo* getInfo() const { return compInfo_; }

    void setVal(BaseComponent* v) { val_ = v; }

    void apply() override { std::cout << "Processing component: " << val_->getName() << std::endl; }
    void Dump([[maybe_unused]] const int verbosity, [[maybe_unused]] std::ios_base::fmtflags base = std::ios_base::dec,
        std::ostream& os = std::cout) override
    {
        if ( verbosity < 3 )
            os << val_->getName() << "/" << std::endl;
        else
            os << val_->getName() << "/ (" << compInfo_->getType() << ")" << std::endl;
    }

    ComponentObj* find(const std::string name)
    {
        ComponentObj* result = nullptr;
        applyRecursiveByType<ComponentObj>([&result, &name](ComponentObj* obj) {
            if ( obj->getVal()->getName() == name ) {
                result = obj;
            }
        });
        return result;
    }
};

template <typename Obj_T>
void
ObjTree<Obj_T>::BuildTree(const ComponentInfoMap& compMap)
{
    if ( !children_.empty() ) {
        std::cout << "WARNING: Calling BuildTree on non-empty ObjTree" << std::endl;
    }
    for ( auto comp = compMap.begin(); comp != compMap.end(); comp++ ) {
        ComponentInfo* compinfo = *comp;
        BaseComponent* bc       = compinfo->getComponent();
        ComponentObj*  c        = new ComponentObj(bc, compinfo);
        addChildObj(c);
    }

    std::sort(children_.begin(), children_.end(),
        [](const std::unique_ptr<ObjTreeCont>& a, const std::unique_ptr<ObjTreeCont>& b) {
            return a->getObjName() < b->getObjName();
        });
}

class ContainerObj : public ObjTree<ContainerObj>
{
    size_t     size_      = 0;
    ObjectMap* sourceMap_ = nullptr;

public:
    ContainerObj(const std::string& name, const std::string& type, size_t size, ObjectMap* source) :
        ObjTree<ContainerObj>(NodeKind::Container),
        size_(size),
        sourceMap_(source)
    {
        setName(name);
        setType(type);
    }

    ContainerObj(const ContainerObj& rhs) :
        ObjTree<ContainerObj>(rhs),
        size_(rhs.size_),
        sourceMap_(rhs.sourceMap_)
    {
        setName(rhs.name_);
        setType(rhs.type_);
    }

    ContainerObj& operator=(const ContainerObj& rhs)
    {
        if ( this == &rhs ) return *this;
        ObjTree<ContainerObj>::operator=(rhs);
        name_      = rhs.name_;
        type_      = rhs.type_;
        size_      = rhs.size_;
        sourceMap_ = rhs.sourceMap_;
        return *this;
    }

    ObjTreeCont* clone() const override { return new ContainerObj(*this); }


    const std::string& getContainerType() const { return type_; }
    size_t             getSize() const { return size_; }

    void syncFromSim() override
    {
        if ( !sourceMap_ ) return;
        size_ = sourceMap_->getVariables().size(); // or cheaper accessor if available
    }

    bool hasChanged() override
    {
        if ( !sourceMap_ ) return false;
        return size_ != sourceMap_->getVariables().size();
    }

    void apply() override { std::cout << "Container: " << name_ << " (" << type_ << ") size=" << size_ << std::endl; }

    void Dump(
        const int verbosity, std::ios_base::fmtflags base = std::ios_base::dec, std::ostream& os = std::cout) override
    {
        if ( verbosity == 0 ) {
            os << name_ << " [" << size_ << " elements] (" << type_ << ")" << std::endl;
        }
        else {
            os << name_ << " [" << size_ << " elements] (" << type_ << ")" << std::endl;
            applyRecursive([&](ObjTreeCont* child) { child->Dump(verbosity - 1, base, os); });
        }
    }

    ObjTreeCont* getElementAt(std::vector<size_t> indices) const
    {
        ObjTreeCont* current = const_cast<ContainerObj*>(this);

        for ( size_t idx : indices ) {
            if ( !current ) return nullptr;

            auto& elems = current->getChildren();
            if ( idx >= elems.size() ) return nullptr;

            current = elems[idx].get();
        }
        return current;
    }

    void printElementAt(std::vector<size_t> indices, int verbosity = 1,
        std::ios_base::fmtflags base = std::ios_base::dec, std::ostream& os = std::cout) const
    {
        ObjTreeCont* elem = getElementAt(indices);
        if ( elem ) {
            elem->Dump(verbosity, base, os);
        }
        else {
            os << "Element not found at path {";
            bool first = true;
            for ( auto i : indices ) {
                if ( !first ) os << ", ";
                os << i;
                first = false;
            }
            os << "}" << std::endl;
        }
    }

    bool setElementFromString(std::vector<size_t> indices, std::string val)
    {
        ObjTreeCont* elem    = getElementAt(indices);
        bool         success = elem != nullptr;
        if ( elem ) {
            success = elem->setFromString(val);
        }
        if ( !success ) {
            std::cout << "Element not found at path {";
            bool first = true;
            for ( auto i : indices ) {
                if ( !first ) std::cout << ", ";
                std::cout << i;
                first = false;
            }
            std::cout << "}" << std::endl;
        }
        return success;
    }
};

// Node for string types (treated specially since they're fundamental-like)
class StringObj : public ObjTree<StringObj>
{
    std::string val_;
    void*       addr_;

public:
    StringObj(const std::string& v, void* addr) :
        ObjTree<StringObj>(NodeKind::String),
        val_(v),
        addr_(addr)
    {}
    StringObj(const StringObj& rhs) :
        ObjTree<StringObj>(rhs),
        val_(rhs.val_),
        addr_(rhs.addr_)
    {}

    StringObj& operator=(const StringObj& rhs)
    {
        if ( this == &rhs ) return *this;
        ObjTree<StringObj>::operator=(rhs);
        val_  = rhs.val_;
        addr_ = rhs.addr_;
        return *this;
    }

    ObjTreeCont* clone() const override { return new StringObj(*this); }

    const std::string& getVal() const { return val_; }
    void               setVal(const std::string& v) { val_ = v; }
    void               setSimVal(const std::string& v)
    {
        val_                              = v;
        *static_cast<std::string*>(addr_) = val_;
    }
    bool setFromString(const std::string& value) override
    {
        if ( readOnly_ ) {
            return false;
        }
        setSimVal(value);
        return true;
    }
    virtual void syncFromSim() override { val_ = *static_cast<std::string*>(addr_); }
    virtual bool hasChanged() override
    {
        if ( !addr_ ) return false;
        return val_ != *static_cast<std::string*>(addr_);
    }

    void apply() override { std::cout << "Processing string: " << val_ << std::endl; }

    void Dump(const int verbosity, [[maybe_unused]] std::ios_base::fmtflags base = std::ios_base::dec,
        std::ostream& os = std::cout) override
    {
        std::string ro = (readOnly_) ? " (ro)" : "";
        if ( verbosity == 0 ) {
            os << getObjName() << ro << std::endl;
        }
        else if ( verbosity == 1 ) {
            os << getObjName() << ro << " = \"" << val_ << "\"" << std::endl;
        }
        else {
            os << getObjName() << ro << " = \"" << val_ << "\"" << " (" << getType() << ")" << std::endl;
        }
    }
};

// Node for bool (separate from integer for clarity)
class BoolObj : public ObjTree<BoolObj>
{
    bool  val_;
    void* addr_; // This points back to the ObjectMap

public:
    BoolObj(bool v, void* addr) :
        ObjTree<BoolObj>(NodeKind::Bool),
        val_(v),
        addr_(addr)
    {}
    BoolObj(const BoolObj& rhs) :
        ObjTree<BoolObj>(rhs),
        val_(rhs.val_),
        addr_(rhs.addr_)
    {}

    BoolObj& operator=(const BoolObj& rhs)
    {
        if ( this == &rhs ) return *this;
        ObjTree<BoolObj>::operator=(rhs);
        val_  = rhs.val_;
        addr_ = rhs.addr_;
        return *this;
    }

    ObjTreeCont* clone() const override { return new BoolObj(*this); }

    bool getVal() const { return val_; }
    void setVal(bool v) { val_ = v; }
    void setSimVal(bool v)
    {
        *(static_cast<bool*>(addr_)) = v;
        val_                         = v;
    }
    bool setFromString(const std::string& value) override
    {
        if ( readOnly_ ) {
            return false;
        }
        setSimVal(value == "true" || value == "1");
        return true;
    }
    virtual void syncFromSim() override { val_ = *(static_cast<bool*>(addr_)); }
    virtual bool hasChanged() override
    {
        if ( !addr_ ) return false;
        return val_ != *static_cast<bool*>(addr_);
    }

    void apply() override { std::cout << "Processing bool: " << (val_ ? "true" : "false") << std::endl; }

    void Dump(const int verbosity, [[maybe_unused]] std::ios_base::fmtflags base = std::ios_base::dec,
        std::ostream& os = std::cout) override
    {
        std::string ro = (readOnly_) ? " (ro)" : "";
        if ( verbosity == 0 ) {
            os << getObjName() << ro << std::endl;
        }
        else if ( verbosity == 1 ) {
            os << getObjName() << ro << " = " << (val_ ? "true" : "false") << std::endl;
        }
        else {
            os << getObjName() << ro << " = " << (val_ ? "true" : "false") << " (" << getType() << ")" << std::endl;
        }
    }
};

class GenericValObj : public ObjTree<GenericValObj>
{
    std::string val_;
    void*       addr_      = nullptr;
    ObjectMap*  sourceMap_ = nullptr; // for write-back via string interface

public:
    GenericValObj(const std::string& val, void* addr, ObjectMap* source) :
        ObjTree<GenericValObj>(NodeKind::GenericVal),
        val_(val),
        addr_(addr),
        sourceMap_(source)
    {}

    GenericValObj(const GenericValObj& rhs) :
        ObjTree<GenericValObj>(rhs),
        val_(rhs.val_),
        addr_(rhs.addr_),
        sourceMap_(rhs.sourceMap_)
    {}

    GenericValObj& operator=(const GenericValObj& rhs)
    {
        if ( this == &rhs ) return *this;
        ObjTree<GenericValObj>::operator=(rhs);
        val_       = rhs.val_;
        addr_      = rhs.addr_;
        sourceMap_ = rhs.sourceMap_;
        return *this;
    }

    ObjTreeCont* clone() const override { return new GenericValObj(*this); }

    const std::string& getVal() const { return val_; }

    // Write-back through ObjectMap's string-based set interface,
    // which knows the real type and handles conversion internally.
    bool setFromString(const std::string& value) override
    {
        if ( readOnly_ ) {
            return false;
        }
        if ( !sourceMap_ ) return false;
        if ( sourceMap_->isReadOnly() ) return false;
        sourceMap_->set(value);
        // Re-read to confirm the value was accepted
        val_ = sourceMap_->get();
        return true;
    }

    // NOTE: we are currently not carrying around sourceMap so this
    //  will ALWAYS return false
    bool hasChanged() override
    {
        if ( !sourceMap_ ) return false;
        return val_ != sourceMap_->get();
    }

    void apply() override { std::cout << "Processing generic: " << val_ << std::endl; }

    void Dump(const int verbosity, [[maybe_unused]] std::ios_base::fmtflags base = std::ios_base::dec,
        std::ostream& os = std::cout) override
    {
        std::string ro = (readOnly_) ? " (ro)" : "";
        if ( verbosity == 0 ) {
            os << getObjName() << ro << std::endl;
        }
        else if ( verbosity == 1 ) {
            os << getObjName() << ro << " = " << val_ << std::endl;
        }
        else {
            os << getObjName() << ro << " = " << val_ << " (" << getType() << ")" << std::endl;
        }
    }
};

} // namespace SST::Core::Serialization

#endif
