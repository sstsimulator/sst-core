// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SHARED_SHAREDMAP_H
#define SST_CORE_SHARED_SHAREDMAP_H

#include "sst/core/shared/sharedObject.h"
#include "sst/core/sst_types.h"

#include <map>

namespace SST {
namespace Shared {

/**
   SharedMap class.  The class is templated to allow for Map of any
   non-pointer type as value.  The type must be serializable.
 */
template <typename keyT, typename valT>
class SharedMap : public SharedObject
{
    static_assert(!std::is_pointer<valT>::value, "Cannot use a pointer type as value with SharedMap");

    // Forward declaration.  Defined below
    class Data;

public:
    SharedMap() : SharedObject(), published(false), data(nullptr) {}

    ~SharedMap()
    {
        // data does not need to be deleted since the
        // SharedObjectManager owns the pointer
    }

    /**
       Initialize the SharedMap.

       @param obj_name Name of the object.  This name is how the
       object is uniquely identified across ranks.

       @param verify_mode Specifies how multiply written data should
       be verified.  Since the underlying map knows if the data has
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
    int initialize(const std::string& obj_name, verify_type v_type = FE_VERIFY)
    {
        if ( data ) {
            Private::getSimulationOutput().fatal(
                CALL_INFO, 1, "ERROR: called initialize() of SharedMap %s more than once\n", obj_name.c_str());
        }

        data    = manager.getSharedObjectData<Data>(obj_name);
        int ret = incShareCount(data);
        data->setVerify(v_type);
        return ret;
    }

    /*** Typedefs and functions to mimic parts of the vector API ***/
    typedef typename std::map<keyT, valT>::const_iterator         const_iterator;
    typedef typename std::map<keyT, valT>::const_reverse_iterator const_reverse_iterator;

    /**
       Get the size of the map.

       @return size of the map
     */
    inline size_t size() const { return data->getSize(); }

    /**
       Tests if the map is empty.

       @return true if map is empty, false otherwise
     */
    inline bool empty() const { return data->map.empty(); }

    /**
       Counts elements with a specific key.  Becuase this is not a
       multimap, it will either return 1 or 0.

       @return Count of elements with specified key
     */
    size_t count(const keyT& k) const { return data->map.count(k); }

    /**
       Searches the container for an element with a key equivalent to
       k and returns an iterator to it if found, otherwise it returns
       an iterator to SharedMap::end().

       @param key key to search for
     */
    const_iterator find(const keyT& key) const { return data->map.find(key); }

    /**
       Get const_iterator to beginning of underlying map
     */
    const_iterator begin() const { return data->map.cbegin(); }

    /**
       Get const_iterator to end of underlying map
     */
    const_iterator end() const { return data->map.cend(); }

    /**
       Get const_reverse_iterator to beginning of underlying map
     */
    const_reverse_iterator rbegin() const { return data->map.crbegin(); }

    /**
       Get const_reverse_iterator to end of underlying map
     */
    const_reverse_iterator rend() const { return data->map.crend(); }

    /**
       Returns an iterator pointing to the first element in the
       container whose key is not considered to go before k (i.e.,
       either it is equivalent or goes after).

       @param key key to compare to
     */
    inline const_iterator lower_bound(const keyT& key) const { return data->map.lower_bound(key); }

    /**
       Returns an iterator pointing to the first element in the
       container whose key is considered to go after k.

       @param key key to compare to
    */
    inline const_iterator upper_bound(const keyT& key) const { return data->map.lower_bound(key); }

    /**
       Indicate that the calling element has written all the data it
       plans to write.  Writing to the map through this instance
       after publish() is called will create an error.
     */
    void publish()
    {
        if ( published ) return;
        published = true;
        incPublishCount(data);
    }

    /**
       Check whether all instances of this SharedMap have called
       publish().  NOTE: Is is possible that this could return true
       one round, but false the next if a new instance of the
       SharedMap was initialized but not published after the last
       call.
     */
    bool isFullyPublished() { return data->isFullyPublished(); }

    /**
       Write data to the map.  This function is thread-safe, as a
       mutex is used to ensure only one write at a time.

       @param key key of the write

       @param value value to be written
     */
    inline void write(const keyT& key, const valT& value)
    {
        if ( published ) {
            Private::getSimulationOutput().fatal(
                CALL_INFO, 1, "ERROR: write to SharedMap %s after publish() was called\n", data->getName().c_str());
        }
        return data->write(key, value);
    }

    /**
       Read data from the map.  This returns a const reference, so is
       read only.  If the key is not in the map, an out_of_range
       exception will be thrown.

       NOTE: This function does not use a mutex, so it is possible to
       get invalid results if another thread is simulateously writing
       to the map.  However, after the init() phase of simulation is
       complete (in setup() and beyond), this is always a safe
       operation.  If reading during init() and you can't guarantee
       that all elements have already written all elements to the
       SharedMap, use mutex_read() to guarantee thread safety.

       @param key key to read

       @return const reference to data referenced by key

       @exception std::out_of_range key is not found in map
     */
    inline const valT& operator[](const keyT& key) const { return data->read(key); }

    /**
       Read data from the map.  This returns a const reference, so
       is read only.  This version of read is always thread safe (@see
       operator[]).  If the key is not in the map, an out_of_range
       exception will be thrown.

       @param key key to read

       @return const reference to data at index

       @exception std::out_of_range key is not found in map
    */
    inline const valT& mutex_read(const keyT& key) const { return data->mutex_read(key); }

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::Shared::SharedObject::serialize_order(ser);
        ser& published;
        switch ( ser.mode() ) {
        case SST::Core::Serialization::serializer::SIZER:
        case SST::Core::Serialization::serializer::PACK:
        {
            std::string name = data->getName();
            ser&        name;
            break;
        }
        case SST::Core::Serialization::serializer::UNPACK:
        {
            std::string name;
            ser&        name;
            data = manager.getSharedObjectData<Data>(name);
            break;
        }
        };
    }
    ImplementSerializable(SST::Shared::SharedMap<keyT, valT>)
private:
    bool  published;
    Data* data;

    class Data : public SharedObjectData
    {

        // Forward declaration.  Defined below
        class ChangeSet;

    public:
        std::map<keyT, valT> map;
        ChangeSet*           change_set;
        verify_type          verify;

        Data() : SharedObjectData() {}
        Data(const std::string& name) : SharedObjectData(name), change_set(nullptr), verify(VERIFY_UNINITIALIZED)
        {
            if ( Private::getNumRanks().rank > 1 ) { change_set = new ChangeSet(name); }
        }

        ~Data()
        {
            if ( change_set ) delete change_set;
        }

        void setVerify(verify_type v_type)
        {
            if ( v_type != verify && verify != VERIFY_UNINITIALIZED ) {
                Private::getSimulationOutput().fatal(
                    CALL_INFO, 1, "ERROR: Two different verify_types specified for SharedMap %s\n", name.c_str());
            }
            verify = v_type;
            if ( change_set ) change_set->setVerify(v_type);
        }

        size_t getSize() const { return map.size(); }

        void update_write(const keyT& key, const valT& value)
        {
            // Don't need to mutex because this is only ever called
            // from one thread at a time, with barrier before and
            // after, or from write(), which does mutex.
            auto success = map.insert(std::make_pair(key, value));
            if ( !success.second ) {
                // Wrote to a key that already existed
                if ( verify != NO_VERIFY && value != success.first->second ) {
                    Private::getSimulationOutput().fatal(
                        CALL_INFO, 1, "ERROR: wrote two different values to same key in SharedMap %s\n", name.c_str());
                }
            }
        }

        void write(const keyT& key, const valT& value)
        {
            std::lock_guard<std::mutex> lock(mtx);
            check_lock_for_write("SharedMap");
            update_write(key, value);
            if ( change_set ) change_set->addChange(key, value);
        }

        // Inline the read since it may be called often during run().
        // This read is not protected from data races in the case
        // where the map may be simulataeously written by another
        // thread.  If there is a danger of simultaneous access
        // during init, use the mutex_read function until after the
        // init phase.
        inline const valT& read(const keyT& key) { return map.at(key); }

        // Mutexed read for use if you are resizing the array as you go
        inline const valT& mutex_read(const keyT& key)
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto                        ret = map.at(key);
            return ret;
        }

        // Functions inherited from SharedObjectData
        virtual SharedObjectChangeSet* getChangeSet() override { return change_set; }
        virtual void                   resetChangeSet() override { change_set->clear(); }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            SharedObjectData::serialize_order(ser);
            ser& map;
        }

        ImplementSerializable(SST::Shared::SharedMap<keyT, valT>::Data);

    private:
        class ChangeSet : public SharedObjectChangeSet
        {

            std::map<keyT, valT> changes;
            verify_type          verify;

            void serialize_order(SST::Core::Serialization::serializer& ser) override
            {
                SharedObjectChangeSet::serialize_order(ser);
                ser& changes;
                ser& verify;
            }

            ImplementSerializable(SST::Shared::SharedMap<keyT, valT>::Data::ChangeSet);

        public:
            // For serialization
            ChangeSet() : SharedObjectChangeSet(), verify(VERIFY_UNINITIALIZED) {}
            ChangeSet(const std::string& name) : SharedObjectChangeSet(name), verify(VERIFY_UNINITIALIZED) {}

            void addChange(const keyT& key, const valT& value) { changes[key] = value; }

            void setVerify(verify_type v_type) { verify = v_type; }

            void applyChanges(SharedObjectDataManager* manager) override
            {
                auto data = manager->getSharedObjectData<Data>(getName());
                data->setVerify(verify);
                for ( auto x : changes ) {
                    data->update_write(x.first, x.second);
                }
            }

            void clear() override { changes.clear(); }
        };
    };
};

} // namespace Shared
} // namespace SST

#endif // SST_CORE_SHARED_SHAREDMAP_H
