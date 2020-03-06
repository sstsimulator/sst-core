// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_INTERPROCESS_TUNNEL_DEF_H
#define SST_CORE_INTERPROCESS_TUNNEL_DEF_H 1

/*
 * This may be compiled into both SST and an Intel Pin3 tool
 * Therefore it has the following restrictions:
 *  - Nothing that uses runtime type info (e.g., dynamic cast)
 *  - No c++11 (auto, nullptr, etc.)
 *  - Includes must all be PinCRT-enabled but not require PinCRT
 */

#include <inttypes.h>
#include <vector>
#include <unistd.h>

#include "sst/core/interprocess/circularBuffer.h"

namespace SST {
namespace Core {
namespace Interprocess {


extern uint32_t globalMMAPIPCCount;
    
/* Internal bookkeeping */
struct InternalSharedData {
    volatile uint32_t expectedChildren;
    size_t shmSegSize;
    size_t numBuffers;
    size_t offsets[0];  // offset[0] points to user region, offset[1]... points to circular buffers
};

/**
 * This class defines a shared-memory region between a master process and
 * one or more child processes
 * Region has three data structures:
 *  - internal bookkeeping (InternalSharedData),
 *  - user defined shared data (ShareDataType)
 *  - multiple circular-buffer queues with entries of type MsgType
 *
 * @tparam ShareDataType  Type to put in the shared data region
 * @tparam MsgType Type of messages being sent in the circular buffers
 */
template<typename ShareDataType, typename MsgType>
class TunnelDef {

    typedef SST::Core::Interprocess::CircularBuffer<MsgType> CircBuff_t;

public:
    /** Create a new tunnel
     *
     * @param numBuffers Number of buffers for which we should tunnel
     * @param bufferSize How large each buffer should be
     * @param expectedChildren Number of child processes that will connect to this tunnel
     */
    TunnelDef(size_t numBuffers, size_t bufferSize, uint32_t expectedChildren) : master(true), shmPtr(NULL) {
        // Locally buffer info
        numBuffs = numBuffers;
        buffSize = bufferSize;
        children = expectedChildren;
        shmSize = calculateShmemSize(numBuffers, bufferSize);
    }
    
    /** Access an existing tunnel
     * Child creates the TunnelDef, reads the shmSize, and then resizes its map accordingly
     * @param sPtr Location of shared memory region
     */
    TunnelDef(void* sPtr) : master(false), shmPtr(sPtr) {
        isd = (InternalSharedData*)shmPtr;
        shmSize = isd->shmSegSize;
    }

    /** Finish setting up a tunnel once the manager knows the correct size of the tunnel
     * and has mmap'd a large enough region for it
     * @param sPtr Location of shared memory region
     * returns number of children left to attach
     */
    uint32_t initialize(void* sPtr) {
        shmPtr = sPtr;

        if (master) {
            nextAllocPtr = (uint8_t*)shmPtr;

            // Reserve space for InternalSharedData
            // Including an offset array entry for each buffer & sharedData structure
            std::pair<size_t, InternalSharedData*> aResult = reserveSpace<InternalSharedData>((1 + numBuffs) * sizeof(size_t));
            isd = aResult.second;
            isd->expectedChildren = children;
            isd->shmSegSize = shmSize;
            isd->numBuffers = numBuffs;

            // Reserve space for ShareDataType structure
            std::pair<size_t, ShareDataType*> bResult = reserveSpace<ShareDataType>(0);
            isd->offsets[0] = bResult.first;
            sharedData = bResult.second;

            // Reserve space for circular buffers
            const size_t cbSize = sizeof(MsgType) * buffSize;
            for (size_t c = 0; c < isd->numBuffers; c++) {
                CircBuff_t* cPtr = NULL;

                std::pair<size_t, CircBuff_t*> cResult = reserveSpace<CircBuff_t>(cbSize);
                isd->offsets[1+c] = cResult.first;
                cPtr = cResult.second;
                if (!cPtr->setBufferSize(buffSize))
                    exit(1); // function prints error message
                circBuffs.push_back(cPtr);
            }
            return isd->expectedChildren;
        } else {
            isd = (InternalSharedData*)shmPtr;
            shmSize = isd->shmSegSize;

            sharedData = (ShareDataType*)((uint8_t*)shmPtr + isd->offsets[0]);

            for ( size_t c = 0 ; c < isd->numBuffers ; c++ ) {
                circBuffs.push_back((CircBuff_t*)((uint8_t*)shmPtr + isd->offsets[c+1]));
            }
            numBuffs = isd->numBuffers;

            return --(isd->expectedChildren);
        }
    }


    /** Destructor */
    virtual ~TunnelDef()
    {
        shutdown();
    }

    /** Clean up a region */
    void shutdown() {
        if ( master ) {
            for (size_t i = 0; i < circBuffs.size(); i++) {
                circBuffs[i]->~CircBuff_t();
            }
        }
        
        if (shmPtr) {
            shmPtr = NULL;
            isd = NULL;
            sharedData = NULL;
            shmSize = 0;
        }
    }

    /** return size of tunnel */
    size_t getTunnelSize() { return shmSize; }

    /** return a pointer to the ShareDataType region */
    ShareDataType* getSharedData() { return sharedData; }

    /** Write data to buffer, blocks until space is available
     * @param buffer which buffer index to write to
     * @param command message to write to buffer
     */
    void writeMessage(size_t  buffer, const MsgType &command) {
        circBuffs[buffer]->write(command);
    }

    /** Read data from buffer, blocks until message received
     * @param buffer which buffer to read from
     * return the message
     */
    MsgType readMessage(size_t buffer) {
        return circBuffs[buffer]->read();
    }

    /** Read data from buffer, non-blocking
     * @param buffer which buffer to read from
     * @param result pointer to return read message at
     * return whether a message was read
     */
    bool readMessageNB(size_t buffer, MsgType *result) {
        return circBuffs[buffer]->readNB(result);
    }

    /** Empty the messages in a buffer
     * @param buffer which buffer to empty
     */
    void clearBuffer(size_t buffer) {
       circBuffs[buffer]->clearBuffer();
    }

    /** return whether this is a master-side tunnel or a child*/
    bool isMaster() { 
        return master; 
    }

private:
    /** Allocate space for a data structure in the shared region
     * @tparam T data structure type to allocate space for
     * @param extraSpace how many extra bytes to reserve with this allocation
     * return offset from shmPtr where structure was allocated and pointer to structure 
     */
    template <typename T>
    std::pair<size_t, T*> reserveSpace(size_t extraSpace = 0)
    {
        size_t space = sizeof(T) + extraSpace;
        if ( (size_t)((nextAllocPtr + space) - (uint8_t*)shmPtr) > shmSize )
            return std::make_pair<size_t, T*>(0, NULL);
        T* ptr = (T*)nextAllocPtr;
        nextAllocPtr += space;
        new (ptr) T();  // Call constructor if need be
        return std::make_pair((uint8_t*)ptr - (uint8_t*)shmPtr, ptr);
    }
    
    /** Calculate the size of the tunnel */
    static size_t calculateShmemSize(size_t numBuffers, size_t bufferSize) {
        long pagesize = sysconf(_SC_PAGESIZE);
        /* Count how many pages are needed, at minimum */
        size_t isd = 1 + ((sizeof(InternalSharedData) + (1+numBuffers)*sizeof(size_t)) / pagesize);
        size_t buffer = 1 + ((sizeof(CircularBuffer<MsgType>) + bufferSize*sizeof(MsgType)) / pagesize);
        size_t shdata = 1 + ((sizeof(ShareDataType) + sizeof(InternalSharedData)) / pagesize);

        /* Alloc 2 extra pages just in case */
        return (2 + isd + shdata + numBuffers*buffer) * pagesize;
    }


protected:
    /** Pointer to the Shared Data Region */
    ShareDataType *sharedData;

    /** Return the number of buffers */
    size_t getNumBuffers() { return numBuffs; }

private:
    bool master;
    void *shmPtr;

    uint8_t *nextAllocPtr;
    size_t shmSize;

    // Local data
    size_t numBuffs;
    size_t buffSize;
    uint32_t children;

    // Shared objects
    InternalSharedData *isd;
    std::vector<CircBuff_t* > circBuffs;
};

}
}
}



#endif
