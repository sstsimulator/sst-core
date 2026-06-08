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
#include "sst/core/serialization/ObjTreeLeaves.h"
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
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_set>
#include <utility>
#include <vector>

namespace SST::Core::Serialization {


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
        if ( verbosity < 3 ) {
            os << val_->getName() << "/" << std::endl;
        }
        else {
            os << val_->getName() << "/ (" << compInfo_->getType() << ")" << std::endl;
        }
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


} // namespace SST::Core::Serialization

#endif
