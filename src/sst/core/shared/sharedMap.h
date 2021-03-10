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

#ifndef SST_CORE_CORE_SHARED_SHAREDMAP_H
#define SST_CORE_CORE_SHARED_SHAREDMAP_H

#include "sst/core/sst_types.h"
#include "sst/core/shared/sharedObject.h"

#include <map>

namespace SST {
namespace Shared {

/**
   SharedMap class.  The class is templated to allow for an array
   of any non-pointer type.  The type must be serializable.
 */
template <typename keyT, typename valT>
class SharedMap : public SharedObject {
    static_assert(!std::is_pointer<valT>::value, "Cannot use a pointer type as value with SharedMap");

    // Forward declaration.  Defined below
    class Data;
    
public:

    SharedMap() :
        published(false), data(nullptr)
    {}

    ~SharedMap() {
        // data does not need to be deleted since the
        // SharedObjectManager owns the pointer
    }


    /**
       Initialize the SharedMap.

       @param obj_name Name of the object.  This name is how the
       object is uniquely identified across ranks.

     */
    void initialize(const std::string& obj_name) {
        if ( data ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: called initialize() of SharedArray %s more than once\n",obj_name.c_str());
        }

        data = manager.getSharedObjectData<Data>(obj_name);
        incShareCount(data);
    }

    /*** Typedefs and functions to mimic parts of the vector API ***/
    typedef typename std::map<keyT,valT>::const_iterator const_iterator;
    typedef typename std::map<keyT,valT>::const_reverse_iterator const_reverse_iterator;

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
    size_t count (const keyT& k) const {
        return data->map.count(k);
    }

    /**
       Get const_iterator to beginning of underlying map
     */
    const_iterator begin() const {
        return data->map.cbegin();
    }
    
    /**
       Get const_iterator to end of underlying map
     */
    const_iterator end() const {
        return data->map.cend();
    }
    
    /**
       Get const_reverse_iterator to beginning of underlying map
     */
    const_reverse_iterator rbegin() const {
        return data->map.crbegin();
    }
    
    /**
       Get const_reverse_iterator to end of underlying map
     */
    const_reverse_iterator rend() const {
        return data->map.crend();
    }
    

    /**
       Indicate that the calling element has written all the data it
       plans to write.  Writing to the map through this instance
       after publish() is called will create an error.
     */
    void publish() {
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
    bool isFullyPublished() {
        return data->isFullyPublished();
    }

    
    /**
       Write data to the map.  This function is thread-safe, as a
       mutex is used to ensure only one write at a time.

       @param key key of the write

       @param value value to be written
     */
    inline void write(const keyT& key, const valT& value) {
        if ( published ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: write to SharedMap %s after publish() was called\n",
                data->getName().c_str());
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
    inline const valT& operator[](const keyT& key) const {
        return data->read(key);
    }

    /**
       Read data from the map.  This returns a const reference, so
       is read only.  This version of read is always thread safe (@see
       operator[]).  If the key is not in the map, an out_of_range
       exception will be thrown.

       @param key key to read

       @return const reference to data at index

       @exception std::out_of_range key is not found in map
    */
    inline const valT& mutex_read(const keyT& key) const {
        return data->mutex_read(key);
    }



    
private:
    bool published;
    Data* data;


    class Data : public SharedObjectData {

        // Forward declaration.  Defined below
        class ChangeSet;

    public:
        
        std::map<keyT,valT> map;
        ChangeSet* change_set;

        Data(const std::string& name) :
            SharedObjectData(name),
            change_set(nullptr)
        {
            if ( Simulation::getSimulation()->getNumRanks().rank > 1 ) {
                change_set = new ChangeSet(name);
            }
        }

        ~Data() {
            delete change_set;
        }


        size_t getSize() const {
            std::lock_guard<std::mutex> lock(mtx);
            return map.size();
        }

        void update_write(const keyT& key, const valT& value) {
            // Don't need to mutex because this is only ever called
            // from one thread at a time, with barrier before and
            // after, or from write(), which does mutex.
            auto success = map.insert(std::make_pair(key,value));
            if ( !success.second ) {
                // Wrote to a key that already existed
                if ( value != success.first->second ) {
                    Simulation::getSimulationOutput().fatal(
                        CALL_INFO, 1, "ERROR: wrote two different values to same key in SharedMap %s\n",name.c_str());
                }
            }
        }

        void write(const keyT& key, const valT& value) {
            std::lock_guard<std::mutex> lock(mtx);
            check_lock_for_write("SharedMap");
            update_write(key,value);
            if ( change_set ) change_set->addChange(key,value);
        }

        // Inline the read since it may be called often during run().
        // This read is not protected from data races in the case where
        // the array may be resized by another thread.  If there is a
        // danger of the array being resized during init, use the
        // mutex_read function until after the init phase.
        inline const valT& read(const keyT& key) {
            return map.at(key);
        }

        // Mutexed read for use if you are resizing the array as you go
        inline const valT& mutex_read(const keyT& key) {
            std::lock_guard<std::mutex> lock(mtx);
            auto ret = map.at(key);
            return ret;
        }

        // Functions inherited from SharedObjectData
        virtual SharedObjectChangeSet* getChangeSet() override {
            return change_set;
        }
        virtual void resetChangeSet() override {
            change_set->clear();
        }

    private:
        class ChangeSet : public SharedObjectChangeSet {

            std::map<keyT,valT> changes;

            void serialize_order(SST::Core::Serialization::serializer& ser) override {
                SharedObjectChangeSet::serialize_order(ser);
                ser & changes;
            }

            ImplementSerializable(SST::Shared::SharedMap<keyT,valT>::Data::ChangeSet);

        public:
            // For serialization
            ChangeSet() :
                SharedObjectChangeSet()
                {}
            ChangeSet(const std::string& name) :
                SharedObjectChangeSet(name)
                {}

            void addChange(const keyT& key, const valT& value) {
                changes[key] = value;
            }

            void applyChanges(SharedObjectDataManager* manager) override {
                auto data = manager->getSharedObjectData<Data>(getName());
                for ( auto x : changes ) {
                    data->update_write(x.first, x.second);
                }
            }

            void clear() override {
                changes.clear();
            }
        };
    };



};

} // namespace Shared
} // namespace SST

#endif
