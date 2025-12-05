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

#ifndef SST_CORE_SERIALIZATION_OBJECTMAPDEFERRED_H
#define SST_CORE_SERIALIZATION_OBJECTMAPDEFERRED_H

#include "sst/core/serialization/serializer.h"

#include <map>
#include <string>
#include <vector>

namespace SST::Core::Serialization {

/**
   ObjectMap version that will delay building the internal data
   structures until the object is "selected".  Also, once the object's
   parent is selected, the internal data will be deleted.
 */
template <typename T>
class ObjectMapDeferred : public ObjectMap
{
public:
    /**
       Returns type of the deferred object

       @return empty type of object
     */
    std::string getType() const final override { return type_; }

    /**
       Returns nullptr since there is no underlying object being
       represented

       @return nullptr
     */
    void* getAddr() const final override { return addr_; }


    /**
       Get the list of child variables contained in this ObjectMap

       @return Reference to map containing ObjectMaps for this
       ObjectMap's child variables. pair.first is the name of the
       variable in the context of this object.
     */
    const ObjectMultimap& getVariables() const final override { return obj_->getVariables(); }

    /**
       For the Deferred Build, the only variable that gets added will
       be the "real" ObjectMap.

       @param name Name of the object
       @param obj ObjectMap to add as a variable
     */
    void addVariable(const std::string& name, ObjectMap* obj) final override
    {
        // This should be the real ObjectMap for the class we are
        // representing. We will check it by making sure the name
        // matches what is passed in in the activate_callback()
        // function.
        if ( name == "!proxy!" ) {
            obj_ = obj;
        }
        else {
            printf("WARNING:: ObjectMapDeferred not built properly.  No mapping will be available\n");
        }
    }

    ObjectMapDeferred(T* addr, const std::string& type) :
        ObjectMap(),
        addr_(addr),
        type_(demangle_name(type.c_str()))
    {}

    ~ObjectMapDeferred() override { delete obj_; }

protected:
    void activate_callback() final override
    {
        // On activate, we need to create the ObjectMap data structure
        if ( obj_ ) return;

        SST::Core::Serialization::serializer ser;
        ser.enable_pointer_tracking();
        ser.start_mapping(this);

        SST_SER_NAME(addr_, "!proxy!");
    }

    void deactivate_callback() final override
    {
        // On deactivate, need to delete obj_;
        obj_->decRefCount();
        obj_ = nullptr;
    }


private:
    /**
       ObjectMap for the real data
     */
    ObjectMap* obj_ = nullptr;

    /**
       Object this ObjectMap represents
     */
    T* addr_ = nullptr;

    /**
       Type of the variable as given by the demangled version of
       typeid(T).name() for the type.
     */
    std::string type_ = "";
};

} // namespace SST::Core::Serialization

#endif // SST_CORE_SERIALIZATION_OBJECTMAPDEFERRED_H
