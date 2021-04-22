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

#ifndef SST_CORE_CORE_SHARED_SHAREDARRAY_H
#define SST_CORE_CORE_SHARED_SHAREDARRAY_H

#include "sst/core/sst_types.h"
#include "sst/core/shared/sharedObject.h"

#include <vector>

namespace SST {
namespace Shared {


/**
   SharedArray class.  The class is templated to allow for an array
   of any non-pointer type.  The type must be serializable.
 */
template <typename T>
class SharedArray : public SharedObject {
    static_assert(!std::is_pointer<T>::value, "Cannot use a pointer type with SharedArray");

    // Forward declaration.  Defined below
    class Data;

public:

    /**
       Default constructor for SharedArray.
    */
    SharedArray() :
        SharedObject(), published(false), data(nullptr)
    {}

    /**
       Shared Array Destructor
     */
    ~SharedArray() {
        // data does not need to be deleted since the
        // SharedObjectManager owns the pointer
    }


    /**
       Initialize the SharedArray.

       @param obj_name Name of the object.  This name is how the
       object is uniquely identified across ranks.

       @param length Length of the array.  The length can be 0 if no
       data is going to be written by the calling element, otherwise,
       it must be large enough to write the desired data.  The final
       length of the array will be the maximum of the requested
       lengths.  The length of the array can be queried using the
       size() method.

       @param init_value value that entries should be initialized to.
       By default, each array item will be initialized using the
       default constructor for the class being stored.

       @param verify_mode Specifies how multiply written data should
       be verified.  FE_VERIFY uses a full/empty bit for each entry
       and if a value has previously been written, it will make sure
       the two values match.  INIT_VERIFY will simply compare writes
       against the current value and will error unless the values
       aren't the same or the existing value is the init_value.  When
       NO_VERIFY is passed, no verification will occur.  This is
       mostly useful when you can guarantee that multiple elements
       won't write the same value and you want to do in-place
       modifications as you initialize.  VERIFY_UNINITIALIZED is a
       reserved value and should not be passed.

       @return returns the number of instances that have intialized
       themselve before this instance on this MPI rank.
     */
    int initialize(const std::string& obj_name, size_t length = 0, T init_value = T(), verify_type v_type = INIT_VERIFY) {
        if ( data ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: called initialize() of SharedArray %s more than once\n",obj_name.c_str());
        }

        if ( v_type == VERIFY_UNINITIALIZED ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: VERIFY_UNINITIALIZED passed into instance of SharedArray %s.  "
                "This is a reserved value and cannot be passed in here. \n",obj_name.c_str());
        }

        data = manager.getSharedObjectData<Data>(obj_name);
        int ret = incShareCount(data);
        if ( length != 0 ) data->setSize(length,init_value,v_type);
        return ret;
    }


    /*** Typedefs and functions to mimic parts of the vector API ***/

    typedef typename std::vector<T>::const_iterator const_iterator;
    typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;

    /**
       Get the length of the array.

       @return length of the array
     */
    inline size_t size() const { return data->getSize(); }


    /**
       Tests if array is empty.

       @return true if array is empty (size = 0), false otherwise
     */
    inline bool empty() const { return data->array.empty(); }


    /**
       Get const_iterator to beginning of underlying map
     */
    const_iterator begin() const {
        return data->array.cbegin();
    }

    /**
       Get const_iterator to end of underlying map
     */
    const_iterator end() const {
        return data->array.cend();
    }


    /**
       Get const_reverse_iterator to beginning of underlying map
     */
    const_reverse_iterator rbegin() const {
        return data->array.crbegin();
    }

    /**
       Get const_reverse_iterator to end of underlying map
     */
    const_reverse_iterator rend() const {
        return data->array.crend();
    }



    /**
       Indicate that the calling element has written all the data it
       plans to write.  Writing to the array through this instance
       after publish() is called will create an error.
     */
    void publish() {
        if ( published ) return;
        published = true;
        incPublishCount(data);
    }

    /**
       Check whether all instances of this SharedArray have called
       publish().  NOTE: Is is possible that this could return true
       one round, but false the next if a new instance of the
       SharedArray was initialized but not published after the last
       call.
     */
    bool isFullyPublished() {
        return data->isFullyPublished();
    }


    /**
       Write data to the array.  This function is thread-safe, as a
       mutex is used to ensure only one write at a time.

       @param index index of the write

       @param value value to be written
     */
    inline void write(int index, const T& value) {
        if ( published ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: write to SharedArray %s after publish() was called\n",
                data->getName().c_str());
        }
        return data->write(index, value);
    }

    /**
       Read data from the array.  This returns a const reference, so
       is read only.

       NOTE: This function does not use a mutex, so it is possible to
       get invalid results if another thread caused a resize of the
       underlying data structure at the same time as the read.
       However, after the init() phase of simulation is complete (in
       setup() and beyond), this is always a safe operation.  If
       reading during init() and you can't guarantee that all elements
       have already written all data to the SharedArray, use
       mutex_read() to guarantee thread safety.

       @param index index to read

       @return const reference to data at index
     */
    inline const T& operator[](int index) const {
        return data->read(index);
    }

    /**
       Read data from the array.  This returns a const reference, so
       is read only.  This version of read is always thread safe (@see
       operator[]).

       @param index index to read

       @return const reference to data at index
     */
    inline const T& mutex_read(int index) const {
        return data->mutex_read(index);
    }


private:
    bool published;
    Data* data;


    class Data : public SharedObjectData {

        // Forward declaration.  Defined below
        class ChangeSet;

    public:

        std::vector<T> array;
        std::vector<bool> written;
        ChangeSet* change_set;
        T init;
        verify_type verify;


        Data(const std::string& name) :
            SharedObjectData(name),
            change_set(nullptr),
            verify(VERIFY_UNINITIALIZED)
            {
                if ( Simulation::getSimulation()->getNumRanks().rank > 1 ) {
                    change_set = new ChangeSet(name);
                }
            }

        ~Data() {
            delete change_set;
        }

        /**
           Set the size of the array.  An element can only write up to the
           current size (reading or writing beyond the size will create
           undefined behavior).  However, an element can put in the size
           it needs for it's writes and it will end up being the largest
           size requested.  We use a vector underneatch the covers to
           manage the memory/copying of data.
        */
        void setSize(size_t size, const T& init_data, verify_type v_type) {
            // If the data is uninitialized, then there is nothing to do
            if ( v_type == VERIFY_UNINITIALIZED ) return;
            std::lock_guard<std::mutex> lock(mtx);
            if ( size > array.size() ) {
                // Need to resize the vector
                array.resize(size,init_data);
                if ( v_type == FE_VERIFY ) {
                    written.resize(size);
                }
                if ( change_set ) change_set->setSize(size, init_data, v_type);
            }
            // init and verify must match across all intances.  We can
            // tell that they have been when verify is not
            // VERIFY_UNINITIALIZED.
            if ( verify != VERIFY_UNINITIALIZED ) {
                if ( init != init_data ) {
                    Simulation::getSimulationOutput().fatal(
                        CALL_INFO,1,"ERROR: Two different init_data values passed into SharedArray %s\n",name.c_str());
                }

                if ( verify != v_type ) {
                    Simulation::getSimulationOutput().fatal(
                        CALL_INFO,1,"ERROR: Two different verify types passed into SharedArray %s\n",name.c_str());
                }
            }
            init = init_data;
            verify = v_type;
        }

        size_t getSize() {
            std::lock_guard<std::mutex> lock(mtx);
            return array.size();
        }

        void update_write(int index, const T& data) {
            // Don't need to mutex because this is only ever called
            // from one thread at a time, with barrier before and
            // after, or from write(), which does mutex.
            bool check = false;
            switch ( verify ) {
            case FE_VERIFY:
                check = written[index];
                break;
            case INIT_VERIFY:
                check = array[index] != init;
                break;
            default:
                break;
            }
            if ( check && (array[index] != data) ) {
                Simulation::getSimulationOutput().fatal(
                    CALL_INFO, 1, "ERROR: wrote two different values to index %d of SharedArray %s\n",index,name.c_str());
            }
            array[index] = data;
            if ( verify == FE_VERIFY ) written[index] = true;
        }

        void write(int index, const T& data) {
            std::lock_guard<std::mutex> lock(mtx);
            check_lock_for_write("SharedArray");
            update_write(index,data);
            if ( verify == FE_VERIFY ) written[index] = true;
            if ( change_set ) change_set->addChange(index,data);
        }

        // Inline the read since it may be called often during run().
        // This read is not protected from data races in the case where
        // the array may be resized by another thread.  If there is a
        // danger of the array being resized during init, use the
        // mutex_read function until after the init phase.
        inline const T& read(int index) const {
            return array[index];
        }

        // Mutexed read for use if you are resizing the array as you go
        inline const T& mutex_read(int index) const {
            std::lock_guard<std::mutex> lock(mtx);
            auto ret = array[index];
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

            std::vector<std::pair<int,T> > changes;
            size_t size;
            T init;
            verify_type verify;

            void serialize_order(SST::Core::Serialization::serializer& ser) override {
                SharedObjectChangeSet::serialize_order(ser);
                ser & changes;
                ser & size;
                ser & init;
                ser & verify;
            }

            ImplementSerializable(SST::Shared::SharedArray<T>::Data::ChangeSet);

        public:
            // For serialization
            ChangeSet() :
                SharedObjectChangeSet()
                {}
            ChangeSet(const std::string& name) :
                SharedObjectChangeSet(name),
                size(0),
                verify(VERIFY_UNINITIALIZED)
                {}

            void addChange(int index, const T& value) {
                changes.emplace_back(index,value);
            }

            void setSize(size_t length, const T& init_data, verify_type v_type) {
                size = length;
                init = init_data;
                verify = v_type;
            }
            size_t getSize() { return size; }

            void applyChanges(SharedObjectDataManager* manager) override {
                auto data = manager->getSharedObjectData<Data>(getName());
                data->setSize(size, init, verify);
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



/**
   SharedArray class.  The class is templated to allow for an array
   of any non-pointer type.  The type must be serializable.
 */
template <>
class SharedArray<bool> : public SharedObject {
    // Forward declaration.  Defined below
    class Data;

public:

    /**
       Default constructor for SharedArray.
    */
    SharedArray() :
        SharedObject(), published(false), data(nullptr)
    {}

    /**
       Shared Array Destructor
     */
    ~SharedArray() {
        // data does not need to be deleted since the
        // SharedObjectManager owns the pointer
    }


    /**
       Initialize the SharedArray.

       @param obj_name Name of the object.  This name is how the
       object is uniquely identified across ranks.

       @param length Length of the array.  The length can be 0 if no
       data is going to be written by the calling element, otherwise,
       it must be large enough to write the desired data.  The final
       length of the array will be the maximum of the requested
       lengths.  The length of the array can be queried using the
       size() method.

       @param init_value value that entries should be initialized to.
       By default, each array item will be initialized using the
       default constructor for the class being stored.

       @param verify_mode Specifies how multiply written data should
       be verified.  FE_VERIFY uses a full/empty bit for each entry
       and if a value has previously been written, it will make sure
       the two values match.  INIT_VERIFY will simply compare writes
       against the current value and will error unless the values
       aren't the same or the existing value is the init_value.  When
       NO_VERIFY is passed, no verification will occur.  This is
       mostly useful when you can guarantee that multiple elements
       won't write the same value and you want to do in-place
       modifications as you initialize.  VERIFY_UNINITIALIZED is a
       reserved value and should not be passed.

       @return returns the number of instances that have intialized
       themselve before this instance on this MPI rank.
     */
    int initialize(const std::string& obj_name, size_t length = 0, bool init_value = false, verify_type v_type = INIT_VERIFY) {
        if ( data ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: called initialize() of SharedArray %s more than once\n",obj_name.c_str());
        }

        if ( v_type == VERIFY_UNINITIALIZED ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: VERIFY_UNINITIALIZED passed into instance of SharedArray %s.  "
                "This is a reserved value and cannot be passed in here. \n",obj_name.c_str());
        }

        data = manager.getSharedObjectData<Data>(obj_name);
        int ret = incShareCount(data);
        if ( length != 0 ) data->setSize(length,init_value,v_type);
        return ret;
    }


    /*** Typedefs and functions to mimic parts of the vector API ***/

    typedef typename std::vector<bool>::const_iterator const_iterator;
    typedef typename std::vector<bool>::const_reverse_iterator const_reverse_iterator;

    /**
       Get the length of the array.

       @return length of the array
     */
    inline size_t size() const { return data->getSize(); }


    /**
       Tests if array is empty.

       @return true if array is empty (size = 0), false otherwise
     */
    inline bool empty() const { return data->array.empty(); }


    /**
       Get const_iterator to beginning of underlying map
     */
    const_iterator begin() const {
        return data->array.cbegin();
    }

    /**
       Get const_iterator to end of underlying map
     */
    const_iterator end() const {
        return data->array.cend();
    }


    /**
       Get const_reverse_iterator to beginning of underlying map
     */
    const_reverse_iterator rbegin() const {
        return data->array.crbegin();
    }

    /**
       Get const_reverse_iterator to end of underlying map
     */
    const_reverse_iterator rend() const {
        return data->array.crend();
    }



    /**
       Indicate that the calling element has written all the data it
       plans to write.  Writing to the array through this instance
       after publish() is called will create an error.
     */
    void publish() {
        if ( published ) return;
        published = true;
        incPublishCount(data);
    }

    /**
       Check whether all instances of this SharedArray have called
       publish().  NOTE: Is is possible that this could return true
       one round, but false the next if a new instance of the
       SharedArray was initialized but not published after the last
       call.
     */
    bool isFullyPublished() {
        return data->isFullyPublished();
    }


    /**
       Write data to the array.  This function is thread-safe, as a
       mutex is used to ensure only one write at a time.

       @param index index of the write

       @param value value to be written
     */
    inline void write(int index, bool value) {
        if ( published ) {
            Simulation::getSimulationOutput().fatal(
                CALL_INFO,1,"ERROR: write to SharedArray %s after publish() was called\n",
                data->getName().c_str());
        }
        return data->write(index, value);
    }

    /**
       Read data from the array.  This returns a const reference, so
       is read only.

       NOTE: This function does not use a mutex, so it is possible to
       get invalid results if another thread caused a resize of the
       underlying data structure at the same time as the read.
       However, after the init() phase of simulation is complete (in
       setup() and beyond), this is always a safe operation.  If
       reading during init() and you can't guarantee that all elements
       have already written all data to the SharedArray, use
       mutex_read() to guarantee thread safety.

       @param index index to read

       @return const reference to data at index
     */
    inline bool operator[](int index) const {
        return data->read(index);
    }

    /**
       Read data from the array.  This returns a const reference, so
       is read only.  This version of read is always thread safe (@see
       operator[]).

       @param index index to read

       @return const reference to data at index
     */
    inline bool mutex_read(int index) const {
        return data->mutex_read(index);
    }


private:
    bool published;
    Data* data;


    class Data : public SharedObjectData {

        // Forward declaration.  Defined below
        class ChangeSet;

    public:

        std::vector<bool> array;
        std::vector<bool> written;
        ChangeSet* change_set;
        bool init;
        verify_type verify;


        Data(const std::string& name) :
            SharedObjectData(name),
            change_set(nullptr),
            verify(VERIFY_UNINITIALIZED)
            {
                if ( Simulation::getSimulation()->getNumRanks().rank > 1 ) {
                    change_set = new ChangeSet(name);
                }
            }

        ~Data() {
            delete change_set;
        }

        /**
           Set the size of the array.  An element can only write up to the
           current size (reading or writing beyond the size will create
           undefined behavior).  However, an element can put in the size
           it needs for it's writes and it will end up being the largest
           size requested.  We use a vector underneatch the covers to
           manage the memory/copying of data.
        */
        void setSize(size_t size, bool init_data, verify_type v_type) {
            // If the data is uninitialized, then there is nothing to do
            if ( v_type == VERIFY_UNINITIALIZED ) return;
            std::lock_guard<std::mutex> lock(mtx);
            if ( size > array.size() ) {
                // Need to resize the vector
                array.resize(size,init_data);
                if ( v_type == FE_VERIFY ) {
                    written.resize(size);
                }
                if ( change_set ) change_set->setSize(size, init_data, v_type);
            }
            // init and verify must match across all intances.  We can
            // tell that they have been when verify is not
            // VERIFY_UNINITIALIZED.
            if ( verify != VERIFY_UNINITIALIZED ) {
                if ( init != init_data ) {
                    Simulation::getSimulationOutput().fatal(
                        CALL_INFO,1,"ERROR: Two different init_data values passed into SharedArray %s\n",name.c_str());
                }

                if ( verify != v_type ) {
                    Simulation::getSimulationOutput().fatal(
                        CALL_INFO,1,"ERROR: Two different verify types passed into SharedArray %s\n",name.c_str());
                }
            }
            init = init_data;
            verify = v_type;
        }

        size_t getSize() {
            std::lock_guard<std::mutex> lock(mtx);
            return array.size();
        }

        void update_write(int index, bool data) {
            // Don't need to mutex because this is only ever called
            // from one thread at a time, with barrier before and
            // after, or from write(), which does mutex.
            bool check = false;
            switch ( verify ) {
            case FE_VERIFY:
                check = written[index];
                break;
            case INIT_VERIFY:
                check = array[index] != init;
                break;
            default:
                break;
            }
            if ( check && (array[index] != data) ) {
                Simulation::getSimulationOutput().fatal(
                    CALL_INFO, 1, "ERROR: wrote two different values to index %d of SharedArray %s\n",index,name.c_str());
            }
            array[index] = data;
            if ( verify == FE_VERIFY ) written[index] = true;
        }

        void write(int index, bool data) {
            std::lock_guard<std::mutex> lock(mtx);
            check_lock_for_write("SharedArray");
            update_write(index,data);
            if ( verify == FE_VERIFY ) written[index] = true;
            if ( change_set ) change_set->addChange(index,data);
        }

        // Inline the read since it may be called often during run().
        // This read is not protected from data races in the case where
        // the array may be resized by another thread.  If there is a
        // danger of the array being resized during init, use the
        // mutex_read function until after the init phase.
        inline bool read(int index) const {
            return array[index];
        }

        // Mutexed read for use if you are resizing the array as you go
        inline bool mutex_read(int index) const {
            std::lock_guard<std::mutex> lock(mtx);
            return array[index];
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

            std::vector<std::pair<int,bool> > changes;
            size_t size;
            bool init;
            verify_type verify;

            void serialize_order(SST::Core::Serialization::serializer& ser) override {
                SharedObjectChangeSet::serialize_order(ser);
                ser & changes;
                ser & size;
                ser & init;
                ser & verify;
            }

            ImplementSerializable(SST::Shared::SharedArray<bool>::Data::ChangeSet);

        public:
            // For serialization
            ChangeSet() :
                SharedObjectChangeSet()
                {}
            ChangeSet(const std::string& name) :
                SharedObjectChangeSet(name),
                size(0),
                verify(VERIFY_UNINITIALIZED)
                {}

            void addChange(int index, bool value) {
                changes.emplace_back(index,value);
            }

            void setSize(size_t length, bool init_data, verify_type v_type) {
                size = length;
                init = init_data;
                verify = v_type;
            }
            size_t getSize() { return size; }

            void applyChanges(SharedObjectDataManager* manager) override {
                auto data = manager->getSharedObjectData<Data>(getName());
                data->setSize(size, init, verify);
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
