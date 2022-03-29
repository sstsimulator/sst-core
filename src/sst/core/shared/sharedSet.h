// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SHARED_SHAREDSET_H
#define SST_CORE_SHARED_SHAREDSET_H

#include "sst/core/shared/sharedObject.h"
#include "sst/core/sst_types.h"

#include <set>

namespace SST {
namespace Shared {

/**
   SharedSet class.  The class is templated to allow for an array
   of any non-pointer type.  The type must be serializable.
 */
template <typename valT>
class SharedSet : public SharedObject
{
    static_assert(!std::is_pointer<valT>::value, "Cannot use a pointer type as value with SharedSet");

    // Forward declaration.  Defined below
    class Data;

public:
    SharedSet() : SharedObject(), published(false), data(nullptr) {}

    ~SharedSet()
    {
        // data does not need to be deleted since the
        // SharedObjectManager owns the pointer
    }

    /**
       Initialize the SharedSet.

       @param obj_name Name of the object.  This name is how the
       object is uniquely identified across ranks.

       @param verify_mode Specifies how multiply written data should
       be verified.  Since the underlying set knows if the data has
       already been written, FE_VERIFY and INIT_VERIFY simply use this
       built-in mechanism to know when an item has previously been
       written.  When these modes are enabled, multiply written data
       must match what was written before.  When NO_VERIFY is passed,
       no verification will occur.  This is mostly useful when you can
       guarantee that multiple elements won't write the same value and
       you want to do in-place modifications as you initialize.
       VERIFY_UNINITIALIZED is a reserved value and should not be
       passed.

       @return returns the number of instances that have intialized
       themselve before this instance on this MPI rank.
     */
    int initialize(const std::string& obj_name, verify_type v_type)
    {
        if ( data ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO, 1, "ERROR: called initialize() of SharedSet %s more than once\n", obj_name.c_str());
        }

        data    = manager.getSharedObjectData<Data>(obj_name);
        int ret = incShareCount(data);
        data->setVerify(v_type);
        return ret;
    }

    /*** Typedefs and functions to mimic parts of the vector API ***/
    typedef typename std::set<valT>::const_iterator         const_iterator;
    typedef typename std::set<valT>::const_reverse_iterator const_reverse_iterator;

    /**
       Get the size of the set.

       @return size of the set
     */
    inline size_t size() const { return data->getSize(); }

    /**
       Tests if the set is empty.

       @return true if set is empty, false otherwise
     */
    inline bool empty() const { return data->set.empty(); }

    /**
       Counts elements with a specific value.  Becuase this is not a
       multiset, it will either return 1 or 0.

       @return Count of elements with specified value
     */
    size_t count(const valT& k) const { return data->set.count(k); }

    /**
       Get const_iterator to beginning of underlying set
     */
    const_iterator begin() const { return data->set.cbegin(); }

    /**
       Get const_iterator to end of underlying set
     */
    const_iterator end() const { return data->set.cend(); }

    /**
       Get const_reverse_iterator to beginning of underlying set
     */
    const_reverse_iterator rbegin() const { return data->set.crbegin(); }

    /**
       Get const_reverse_iterator to end of underlying set
     */
    const_reverse_iterator rend() const { return data->set.crend(); }

    /**
       Indicate that the calling element has written all the data it
       plans to write.  Writing to the set through this instance
       after publish() is called will create an error.
     */
    void publish()
    {
        if ( published ) return;
        published = true;
        incPublishCount(data);
    }

    /**
       Check whether all instances of this SharedSet have called
       publish().  NOTE: Is is possible that this could return true
       one round, but false the next if a new instance of the
       SharedSet was initialized but not published after the last
       call.
     */
    bool isFullyPublished() { return data->isFullyPublished(); }

    /**
       Insert data to the set.  This function is thread-safe, as a
       mutex is used to ensure only one insert at a time.

       @param val value of the insert
     */
    inline void insert(const valT& value)
    {
        if ( published ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO, 1, "ERROR: insert into SharedSet %s after publish() was called\n", data->getName().c_str());
        }
        return data->write(value);
    }

    /**
       Searches the SharedSet for an element equivalent to value and
       returns a const iterator to it if found, otherwise it returns
       an iterator to SharedSet::end.

       @param value value to search for

       NOTE: This function does not use a mutex, so it is possible to
       get invalid results if another thread is simulateously writing
       to the set.  However, after the init() phase of simulation is
       complete (in setup() and beyond), this is always a safe
       operation.  If reading during init() and you can't guarantee
       that all elements have already written all elements to the
       SharedSet, use mutex_find() to guarantee thread safety.

       @return read-only iterator to data referenced by value
     */
    inline const_iterator find(const valT& value) const { return data->find(value); }

    /**
       Searches the SharedSet for an element equivalent to value and
       returns a const iterator to it if found, otherwise it returns
       an iterator to SharedSet::end. This version of find is always thread safe (@see
       find()).

       @param value value to search for

       @return read-only iterator to data reference by value
    */
    inline const valT& mutex_find(const valT& value) const { return data->mutex_read(value); }

private:
    bool  published;
    Data* data;

    class Data : public SharedObjectData
    {

        // Forward declaration.  Defined below
        class ChangeSet;

    public:
        std::set<valT> set;
        ChangeSet*     change_set;

        verify_type verify;

        Data(const std::string& name) : SharedObjectData(name), change_set(nullptr), verify(VERIFY_UNINITIALIZED)
        {
            if ( Simulation::getSimulation()->getNumRanks().rank > 1 ) { change_set = new ChangeSet(name); }
        }

        ~Data() { delete change_set; }

        void setVerify(verify_type v_type)
        {
            if ( v_type != verify && verify != VERIFY_UNINITIALIZED ) {
                Simulation::getSimulationOutput().fatal(
                    CALL_INFO, 1, "ERROR: Type different verify_types specified for SharedSet %s\n", name.c_str());
            }
            verify = v_type;
            if ( change_set ) change_set->setVerify(v_type);
        }

        size_t getSize() const
        {
            std::lock_guard<std::mutex> lock(mtx);
            return set.size();
        }

        void update_write(const valT& value)
        {
            // Don't need to mutex because this is only ever called
            // from one thread at a time, with barrier before and
            // after, or from write(), which does mutex.
            auto success = set.insert(value);
            if ( !success.second ) {
                // Wrote to a value that already existed
                if ( verify != NO_VERIFY && !(value == *(success.first)) ) {
                    Simulation::getSimulationOutput().fatal(
                        CALL_INFO, 1, "ERROR: wrote two non-equal values to same set item in SharedSet %s\n",
                        name.c_str());
                }
            }
        }

        void write(const valT& value)
        {
            // std::lock_guard<std::mutex> lock(mtx);
            check_lock_for_write("SharedSet");
            update_write(value);
            if ( change_set ) change_set->addChange(value);
        }

        // Inline the read since it may be called often during run().
        // This read is not protected from data races in the case
        // where the set may be simulataeously written by another
        // thread.  If there is a danger of simultaneous access
        // during init, use the mutex_read function until after the
        // init phase.
        inline const_iterator find(const valT& value) { return set.find(value); }

        // Mutexed read for use if you are resizing the array as you go
        inline const valT& mutex_find(const valT& value)
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto                        ret = set.find(value);
            return ret;
        }

        // Functions inherited from SharedObjectData
        virtual SharedObjectChangeSet* getChangeSet() override { return change_set; }
        virtual void                   resetChangeSet() override { change_set->clear(); }

    private:
        class ChangeSet : public SharedObjectChangeSet
        {

            std::set<valT> changes;
            verify_type    verify;

            void serialize_order(SST::Core::Serialization::serializer& ser) override
            {
                SharedObjectChangeSet::serialize_order(ser);
                ser& changes;
                ser& verify;
            }

            ImplementSerializable(SST::Shared::SharedSet<valT>::Data::ChangeSet);

        public:
            // For serialization
            ChangeSet() : SharedObjectChangeSet(), verify(VERIFY_UNINITIALIZED) {}
            ChangeSet(const std::string& name) : SharedObjectChangeSet(name), verify(VERIFY_UNINITIALIZED) {}

            void addChange(const valT& value) { changes.insert(value); }

            void setVerify(verify_type v_type) { verify = v_type; }

            void applyChanges(SharedObjectDataManager* manager) override
            {
                auto data = manager->getSharedObjectData<Data>(getName());
                data->setVerify(verify);
                for ( auto x : changes ) {
                    data->update_write(x);
                }
            }

            void clear() override { changes.clear(); }
        };
    };
};

} // namespace Shared
} // namespace SST

#endif // SST_CORE_SHARED_SHAREDSET_H
