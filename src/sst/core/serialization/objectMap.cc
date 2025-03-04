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

#include "sst_config.h"

#include "sst/core/serialization/objectMap.h"

#include "sst/core/stringize.h"

#include <cxxabi.h>

namespace SST::Core::Serialization {

// Static variable instantiation
std::vector<std::pair<std::string, ObjectMap*>> ObjectMap::emptyVars;


std::string
ObjectMap::getName()
{
    if ( mdata_ ) { return mdata_->name; }
    return "";
}

std::string
ObjectMap::getFullName()
{
    if ( !mdata_ ) return "";

    std::string        fullname = mdata_->name;
    std::string        slash("/");
    ObjectMapMetaData* curr = mdata_->parent->mdata_;
    while ( curr != nullptr ) {
        fullname = curr->name + slash + fullname;
        curr     = curr->parent->mdata_;
    }
    return fullname;
}

ObjectMap*
ObjectMap::selectParent()
{
    if ( nullptr == mdata_ ) return nullptr;
    ObjectMap* ret = mdata_->parent;
    if ( nullptr == ret ) { return nullptr; }
    deactivate();
    return ret;
}

ObjectMap*
ObjectMap::selectVariable(std::string name, bool& loop_detected)
{
    loop_detected  = false;
    ObjectMap* var = findVariable(name);

    // If we get nullptr back, then it didn't exist.  Just return this
    if ( nullptr == var ) return this;

    // See if this creates a loop

    if ( var->mdata_ ) {
        loop_detected      = true;
        // Loop detected.  We will need to remove all the
        // mdata elements from this object back up from
        // parent to parent until we get to the object we
        // are selecting.  This will reset the hierachy to
        // the more shallow one.
        ObjectMap* current = this;
        ObjectMap* parent  = mdata_->parent;
        // delete current->mdata_;
        // current->mdata_ = nullptr;
        current->deactivate();
        while ( parent != var ) {
            // TODO: check for parent == nullptr, which
            // would be the case where we didn't detect
            // the loop going back up the chain. This
            // would mean the metadata was corrupted
            // somehow.
            current = parent;
            parent  = current->mdata_->parent;
            // Clear the metadata for current
            // delete current->mdata_;
            // current->mdata_ = nullptr;
            current->deactivate();
        }
        return var;
    }

    // No loop, set the metadata and return it
    // var->mdata_ = new ObjectMapMetaData(this, name);
    var->activate(this, name);
    return var;
}


std::string
ObjectMap::get(const std::string& var)
{
    bool       loop_detected = false;
    ObjectMap* obj           = selectVariable(var, loop_detected);
    if ( nullptr == obj ) return "";
    if ( !obj->isFundamental() ) return "";
    std::string ret = obj->get();
    obj->selectParent();
    return ret;
}

void
ObjectMap::set(const std::string& var, const std::string& value, bool& found, bool& read_only)
{
    bool       loop_detected = false;
    ObjectMap* obj           = selectVariable(var, loop_detected);
    if ( nullptr == obj ) {
        found = false;
        return;
    }
    found = true;
    if ( obj->isReadOnly() ) {
        read_only = true;
        return;
    }
    read_only = false;
    obj->set(value);
    obj->selectParent();
}


std::string
ObjectMap::demangle_name(const char* name)
{
    int status;

    char* demangledName = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if ( status == 0 ) {
        std::string ret(demangledName);
        std::free(demangledName);
        return ret;
    }
    else {
        return "";
    }
}

std::string
ObjectMap::listVariable(std::string name, bool& found, int recurse)
{
    ObjectMap* var = findVariable(name);
    if ( nullptr == var ) {
        found = false;
        return "";
    }
    found = true;

    std::string ret;

    // Check to see if this is a loopback
    if ( nullptr != var->mdata_ ) {
        // Found a loop
        ret = format_string("%s (%s) = <loopback>\n", name.c_str(), var->getType().c_str());
        return ret;
    }
    var->activate(this, name);
    ret = var->listRecursive(name, 0, recurse);
    var->deactivate();
    return ret;
}

std::string
ObjectMap::list(int recurse)
{
    return listRecursive(mdata_->name, 0, recurse);
}

// Private functions
std::string
ObjectMap::listRecursive(const std::string& name, int level, int recurse)
{
    std::string ret;
    std::string indent = std::string(level, ' ');
    if ( isFundamental() ) {
        ret = format_string("%s%s = %s (%s)\n", indent.c_str(), name.c_str(), get().c_str(), getType().c_str());
        return ret;
    }

    ret = format_string("%s%s (%s)\n", indent.c_str(), name.c_str(), getType().c_str());

    if ( level <= recurse ) {
        for ( auto& x : getVariables() ) {
            bool loop = (nullptr != x.second->mdata_);
            if ( loop ) {
                ret += format_string(
                    "%s %s (%s) = <loopback>\n", indent.c_str(), x.first.c_str(), x.second->getType().c_str());
            }
            else {
                x.second->activate(this, name);
                ret += x.second->listRecursive(x.first, level + 1, recurse);
                x.second->deactivate();
            }
        }
    }
    return ret;
}

} // namespace SST::Core::Serialization
