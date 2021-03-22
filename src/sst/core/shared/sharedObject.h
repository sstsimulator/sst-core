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

#ifndef SST_CORE_CORE_SHARED_SHAREDOBJECT_H
#define SST_CORE_CORE_SHARED_SHAREDOBJECT_H

#include "sst/core/sst_types.h"

#include "sst/core/simulation.h"
#include "sst/core/serialization/serializable.h"

#include <string>

namespace SST {
namespace Shared {

// NOTE: The classes in this header file are not part of the public
// API and can change at any time


class SharedObjectDataManager;

/**
   This is the base class for holding data on changes made to the
   shared data on each rank.  This data will be serialized and shared
   with all ranks in the simulation.
 */
class SharedObjectChangeSet : public SST::Core::Serialization::serializable {

public:
    SharedObjectChangeSet() {}
    SharedObjectChangeSet(const std::string& name) :
        name(name)
    {}

    /**
       Apply the changes to the name shared data.

       @param manager The SharedObjectDataManager for the rank.  This
       is used to get the named shared data.
     */
    virtual void applyChanges(SharedObjectDataManager* UNUSED(manager)) = 0;

    /**
       Clears the data.  Used after transfering data to other ranks to
       prepare for the next round of init.  Child classes should call
       this version of clear() in there implementations.
     */
    virtual void clear() = 0;

    /**
       Get the name of the shared data the changeset should be applied
       to

       @return name of shared data to apply changes to
     */
    const std::string& getName() { return name; }


protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        ser & name;
    }

    ImplementVirtualSerializable(SharedObjectChangeSet);


private:
    std::string name;
};

/**
   Base class for holding SharedObject data.  The base class is needed
   so that we can have an API to manage unknown types of objects.
*/
class SharedObjectData {

    friend class SharedObjectDataManager;
    friend class SharedObject;

public:

    /**
       Get the name of the SharedObject for this data

       @return name of data
     */
    const std::string& getName() { return name; }

    /**
       Checks to see if all instances of this SharedObject have called
       publish().  This is forced to true before the setup() phase of
       simulation, as no more writes are allowed.

       @return true if all instances have called publish(), false
       otherwise.
     */
    bool isFullyPublished() { return fully_published; }

    /**
       Get the number of sharers for this data

       @return number of sharers
     */
    virtual int getShareCount() { return share_count; }

    /**
       Get the number of instances that have called publish() on their
       instance of the shared object

       @return number of instances that have called publish()
     */
    virtual int getPublishCount() { return publish_count; }

protected:

    std::string name;
    int share_count;
    int publish_count;
    bool fully_published;
    bool locked;

    // Mutex for locking across threads
    std::mutex mtx;

    // Check to see if object data is locked.  If so, fatal
    inline void check_lock_for_write(const std::string& obj) {
        if ( locked ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO_LONG,1,"ERROR: attempt to write to %s %s after init phase\n",obj.c_str(),name.c_str());
        }
    }

    /* Manage counts */

    /**
       Increment the count of sharers.  This should only be called
       once per instance.
     */
    virtual void incShareCount() { share_count++; }
    /**
       Increment the count of instances that have called publish.
       This should only be called once per instance.
     */
    virtual void incPublishCount() { publish_count++; }


    /* For merging across ranks */

    /**
       Gets the changeset for this data on this rank.  This is called
       by the core when exchanging and merging data
     */
    virtual SharedObjectChangeSet* getChangeSet() = 0;

    /**
       Resets the changeset for this data on this rank.  This is called
       by the core when exchanging and merging data
     */
    virtual void resetChangeSet() = 0;

    /**
       Called by the core when writing to shared regions is no longer
       allowed
     */
    void lock() { locked = true; }


    /**
       Constructor for SharedObjectData

       @param name name of the SharedObject
     */
    SharedObjectData(const std::string& name) :
        name(name),
        share_count(0),
        publish_count(0),
        fully_published(false),
        locked(false)
    {}

    /**
       Destructor for SharedObjectData
     */
    virtual ~SharedObjectData() {}
};


class SharedObjectDataManager {

    std::map<std::string, SharedObjectData*> shared_data;

    // Mutex for locking across threads
    static std::mutex mtx;
    static std::mutex update_mtx;

    bool locked;

public:

    SharedObjectDataManager() :
        locked(false) {}

    ~SharedObjectDataManager() {
        for ( auto x : shared_data ) {
            delete x.second;
        }
    }

    template <typename T>
    T* getSharedObjectData(const std::string name)
    {
        // Need to lock since we may be initializing the
        // SharedData object.
        std::lock_guard<std::mutex> lock(mtx);
        if ( locked ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"Attempting to initialize SharedObject %s after the init() phase\n",name.c_str());
        }
        auto obj = shared_data.find(name);
        if ( obj == shared_data.end() ) {
            // Does not yet exist, create it
            auto ret = new T(name);
            shared_data[name] = ret;
            return ret;
        }

        // If it exists, make sure the types match
        auto obj_cast = dynamic_cast<T*>(obj->second);

        if ( obj_cast == nullptr ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: Shared object %s requested with two different types\n",name.c_str());
        }

        return obj_cast;
    }

    void updateState(bool finalize);

};


class SharedObject {


public:

    /**
       Enum of verify types.
    */
    enum verify_type { VERIFY_UNITIALIZED, FE_VERIFY, INIT_VERIFY, NO_VERIFY };



    SharedObject() {}
    virtual ~SharedObject() {}

protected:
    friend class SST::Simulation;
    static SharedObjectDataManager manager;


    void incPublishCount(SharedObjectData* data) {
        data->incPublishCount();
    }

    void incShareCount(SharedObjectData* data) {
        data->incShareCount();
    }


};

} // namespace Shared
} // namespace SST

#endif
