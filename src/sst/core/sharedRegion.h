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

#ifndef SST_CORE_SHAREDREGION_H
#define SST_CORE_SHAREDREGION_H

#include "sst/core/sst_types.h"
#include "sst/core/warnmacros.h"

#include <string>
#include <vector>

#if !SST_BUILDING_CORE
#warning \
    "SharedRegion and its accompanying classes have been deprecated and will be removed in SST 12. Please use the new SharedObject classes found in sst/core/shared."
#endif

namespace SST {

class SharedRegion;
class ChangeSet;

/**
 * Utility class to define how to merge multiple pieces of shared memory regions
 * Useful in the multi-MPI-rank, "Global Shared" model
 */
class SharedRegionMerger
{
public:
    virtual ~SharedRegionMerger() {}

    /**
     * Merge the data from 'newData' into 'target'
     * @return True on success, False on failure
     */
    virtual bool merge(uint8_t* target, const uint8_t* newData, size_t size);
    virtual bool merge(uint8_t* target, size_t size, const std::vector<ChangeSet>& changeSets);
};

class SharedRegionInitializedMerger : public SharedRegionMerger
{
    uint8_t defVal;

public:
    SharedRegionInitializedMerger(uint8_t defaultValue) { defVal = defaultValue; }

    bool merge(uint8_t* target, const uint8_t* newData, size_t size) override;

    bool merge(uint8_t* target, size_t size, const std::vector<ChangeSet>& changeSets) override
    {
        return SharedRegionMerger::merge(target, size, changeSets);
    }
};

class SharedRegionManager
{
protected:
    friend class SharedRegion;

    virtual void        modifyRegion(SharedRegion* sr, size_t offset, size_t length, const void* data) = 0;
    virtual void*       getMemory(SharedRegion* sr)                                                    = 0;
    virtual const void* getConstPtr(const SharedRegion* sr) const                                      = 0;
    virtual size_t      getSize(const SharedRegion* sr) const                                          = 0;

public:
    /**
       Create a SharedRegion that will only be shared with elements on
       the current rank.  This type of SharedRegion is intended to
       have at least one element on each rank initialize the region.

       @param key Name of the SharedRegion.  All elements om the rank
       using this name will get the same underlying data.

       @param size Size of the SharedRegion.  Elements can specify 0
       size if they don't know the size of the region.  At least one
       element will need to specify the size and all elements that
       specify a size must pass in the same size.  There are many
       calls in SharedRegion that can't be called until the size is
       known.

       @param initByte Value to initialize all bytes in the region to

       @return SharedRegion object to be used by the calling element
       to access the shared region.
     */
    virtual SharedRegion* getLocalSharedRegion(const std::string& key, size_t size, uint8_t initByte = 0) = 0;

    /**
       Create a SharedRegion that will be shared with elements on the
       all ranks.  The SharedRegion data will be merged across ranks
       before each round of calls to init().

       @param key Name of the SharedRegion.  All elements using this
       name will get the same underlying data.

       @param size Size of the SharedRegion.  Elements can specify 0
       size if they don't know the size of the region.  At least one
       element will need to specify the size and all elements that
       specify a size must pass in the same size.  There are many
       calls in SharedRegion that can't be called until the size is
       known.

       @param merger Merger to use when combining data across ranks

       @param initByte Value to initialize all bytes in the region to

       @return SharedRegion object to be used by the calling element
       to access the shared region.
    */
    virtual SharedRegion* getGlobalSharedRegion(
        const std::string& key, size_t size, SharedRegionMerger* merger = nullptr, uint8_t initByte = 0) = 0;

    // The following functions will become protected in SST 12.  Need
    // to look for instances of these used in the core, they will be
    // bracketed with DISABLE_WARN_DEPRECATED_DECLARATION.
    virtual void publishRegion(SharedRegion*) = 0;

    virtual bool isRegionReady(const SharedRegion*) = 0;

    virtual void shutdownSharedRegion(SharedRegion*) = 0;

    virtual void updateState(bool finalize) = 0;
};

class SharedRegion
{
private:
    SharedRegionManager* manager;
    size_t               id;

protected:
    class DeferredPointerBase
    {
    public:
        virtual ~DeferredPointerBase() {}
        virtual void setPointer(const void* p) = 0;
    };

    template <typename T>
    class DeferredPointer : public DeferredPointerBase
    {
        T& ptr;

    public:
        DeferredPointer(T& ptr) : DeferredPointerBase(), ptr(ptr) {}

        void setPointer(const void* p) { ptr = static_cast<const T>(p); }
    };

    DeferredPointerBase* deferred_pointer;

    SharedRegion(SharedRegionManager* manager, size_t id) : manager(manager), id(id), deferred_pointer(nullptr) {}

public:
    virtual ~SharedRegion() {}

    void shutdown()
    {
        DISABLE_WARN_DEPRECATED_DECLARATION;
        manager->shutdownSharedRegion(this);
        REENABLE_WARNING;
    }

    /**
     * @return The ID of this instance. (Number in range 0->N)
     */
    size_t getLocalShareID() const { return id; }

    /**
     * @return The size of the shared memory region, may return 0 if
     * this copy of the region doesn't know the final size yet.
     */
    size_t getSize() const { return manager->getSize(this); }

    /**
     * Call to denote that you are done making any changes to this region
     */
    void publish()
    {
        DISABLE_WARN_DEPRECATED_DECLARATION;
        manager->publishRegion(this);
        REENABLE_WARNING;
    }

    /**
     * Check to see if the region is ready (all elements requesting
     * the region have called publish()).
     *
     * @return True if the region is ready to use (all sharers have
     * called publish()).
     */
    bool isReady() const
    {
        DISABLE_WARN_DEPRECATED_DECLARATION;
        return manager->isRegionReady(this);
        REENABLE_WARNING;
    }

    /**
     * Before the region has published, apply a modification. (Copy
     * this data in).  This function cannot be called if the size is
     * still set to 0.
     */
    void modifyRegion(size_t offset, size_t length, const void* data)
    {
        manager->modifyRegion(this, offset, length, data);
    }

    /**
     * Before the region has published, apply a modification. (Copy
     * this data in).  This function cannot be called if the size is
     * still set to 0.
     */
    template <typename T>
    void modifyArray(size_t offset, const T& data)
    {
        manager->modifyRegion(this, offset * sizeof(T), sizeof(T), &data);
    }

    /**
     * @return a void* pointer to the shared memory region This
     * pointer is only valid to write to before a call to publish().
     * This function cannot be called if the size is still set to
     * 0, as there is no backing memory yet.
     */
    void* getRawPtr() { return manager->getMemory(this); }

    /**
     * Get a pointer to the const shared data
     *
     * @return a const pointer to the shared memory region.  This
     * function cannot be called if the size is still set to
     * 0, as there is no backing memory yet.
     */
    template <typename T>
    T getPtr() const
    {
        return static_cast<T>(manager->getConstPtr(this));
    }

    /**
     * Used to get a pointer to the const shared data when the element
     * doesn't yet know the size.  This will defer setting the pointer
     * until the size of the region is known (and therefor the memory
     * has been allocated).  This allows an element to initialize the
     * region and get the pointer without having to make the call to
     * getPtr() later.  This is particularly helpful if the
     * SharedRegion is created in an object that doesn't get called
     * during init() or setup().
     *
     * @ptr Pointer that should be set once the size is known.  User
     * can tell pointer is valid when getSize() returns a value != 0.
     */
    template <typename T>
    void getPtrDeferred(T& ptr)
    {
        if ( getSize() != 0 ) {
            // Size is already set, just set the pointer
            ptr = getPtr<T>();
            return;
        }
        if ( deferred_pointer != nullptr ) return;
        deferred_pointer = new DeferredPointer<T>(ptr);
    }
};

} // namespace SST

#endif // SST_CORE_SHAREDREGION_H
