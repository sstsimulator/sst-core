// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_INTERPROCESS_TUNNEL_H
#define SST_CORE_INTERPROCESS_TUNNEL_H 1


#include <fcntl.h>
#include <cstdio>
#include <vector>
#include <string>
#include <errno.h>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sst/core/interprocess/circularBuffer.h>

namespace SST {
namespace Core {
namespace Interprocess {


extern uint32_t globalIPCTunnelCount;
/**
 * Tunneling class between two processes, connected by shared memory.
 * Supports multiple circular-buffer queues, and a generic region
 * of memory for shared data.
 *
 * @tparam ShareDataType  Type to put in the shared data region
 * @tparam MsgType Type of messages being sent in the circular buffers
 */
template<typename ShareDataType, typename MsgType>
class IPCTunnel {

    typedef SST::Core::Interprocess::CircularBuffer<MsgType> CircBuff_t;

    struct InternalSharedData {
        volatile uint32_t expectedChildren;
        size_t shmSegSize;
        size_t numBuffers;
        size_t offsets[0];  // Actual size:  numBuffers + 2
    };


public:
    /**
     * Construct a new Tunnel for IPC Communications
     * @param comp_id Component ID of owner
     * @param numBuffers Number of buffers for which we should tunnel
     * @param bufferSize How large each core's buffer should be
     */
    IPCTunnel(uint32_t comp_id, size_t numBuffers, size_t bufferSize, uint32_t expectedChildren = 1) : master(true), shmPtr(NULL), fd(-1)
    {
        char key[256];
        memset(key, '\0', sizeof(key));
        do {
            snprintf(key, sizeof(key), "/sst_shmem_%u-%" PRIu32 "-%d", getpid(), comp_id, rand());
            filename = key;

            fd = shm_open(filename.c_str(), O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
            /* There's a rare chance that a file we are looking to use exists.
             * It's unlikely, but perhaps a previous run (with the same PID
             * and random number) crashed before the * clients all connected.
             *
             * So, if we get an error, and the error is EEXIST, try again with
             * a different random number.
             */
        } while ( (fd < 0) && (errno == EEXIST) );
        if ( fd < 0 ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "Failed to create IPC region '%s': %s\n", filename.c_str(), strerror(errno));
            exit(1);
        }


        shmSize = calculateShmemSize(numBuffers, bufferSize);
        if ( ftruncate(fd, shmSize) ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "Resizing shared file '%s' failed: %s\n", filename.c_str(), strerror(errno));
            exit(1);
        }

        shmPtr = mmap(NULL, shmSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "mmap failed: %s\n", strerror(errno));
            exit(1);
        }
        nextAllocPtr = (uint8_t*)shmPtr;
        memset(shmPtr, '\0', shmSize);

        /* Construct our private buffer first.  Used for our communications */
        auto resResult = reserveSpace<InternalSharedData>((1+numBuffers)*sizeof(size_t));
        isd = resResult.second;
        isd->expectedChildren = expectedChildren;
        isd->shmSegSize = shmSize;
        isd->numBuffers = numBuffers;

        /* Construct user's shared-data region */
        auto shareResult = reserveSpace<ShareDataType>(0);
        isd->offsets[0] = shareResult.first;
        sharedData = shareResult.second;

        /* Construct the circular buffers */
        const size_t cbSize = sizeof(MsgType) * bufferSize;
        for ( size_t c = 0 ; c < isd->numBuffers ; c++ ) {
            CircBuff_t* cPtr = NULL;

            auto resResult = reserveSpace<CircBuff_t>(cbSize);
            isd->offsets[1+c] = resResult.first;
            cPtr = resResult.second;
            cPtr->setBufferSize(bufferSize);
            circBuffs.push_back(cPtr);
        }

    }

    /**
     * Access an existing Tunnel
     * @param region_name Name of the shared-memory region to access
     */
    IPCTunnel(const std::string &region_name) : master(false), shmPtr(NULL), fd(-1)
    {
        fd = shm_open(region_name.c_str(), O_RDWR, S_IRUSR|S_IWUSR);
        filename = region_name;

        if ( fd < 0 ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "Failed to open IPC region '%s': %s\n",
                    filename.c_str(), strerror(errno));
            exit(1);
        }

        shmPtr = mmap(NULL, sizeof(InternalSharedData), PROT_READ, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "mmap 0 failed: %s\n", strerror(errno));
            exit(1);
        }

        isd = (InternalSharedData*)shmPtr;
        shmSize = isd->shmSegSize;
        munmap(shmPtr, sizeof(InternalSharedData));

        shmPtr = mmap(NULL, shmSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "mmap 1 failed: %s\n", strerror(errno));
            exit(1);
        }
        isd = (InternalSharedData*)shmPtr;
        sharedData = (ShareDataType*)((uint8_t*)shmPtr + isd->offsets[0]);

        for ( size_t c = 0 ; c < isd->numBuffers ; c++ ) {
            circBuffs.push_back((CircBuff_t*)((uint8_t*)shmPtr + isd->offsets[c+1]));
        }

        /* Clean up if we're the last to attach */
        if ( --isd->expectedChildren == 0 ) {
            shm_unlink(filename.c_str());
        }
    }


    /**
     * Destructor
     */
    virtual ~IPCTunnel()
    {
        shutdown(true);
    }

    /**
     * Shutdown
     */
    void shutdown(bool all = false)
    {
        if ( master ) {
            for ( CircBuff_t *cb : circBuffs ) {
                cb->~CircBuff_t();
            }
        }
        if ( shmPtr ) {
            munmap(shmPtr, shmSize);
            shmPtr = NULL;
            shmSize = 0;
        }
        if ( fd >= 0 ) {
            close(fd);
            fd = -1;
        }
    }

    const std::string& getRegionName(void) const { return filename; }

    /** return a pointer to the ShareDataType region */
    ShareDataType* getSharedData() { return sharedData; }

    /** Blocks until space is available **/
    void writeMessage(size_t core, const MsgType &command) {
        circBuffs[core]->write(command);
    }

    /** Blocks until a command is available **/
    MsgType readMessage(size_t buffer) {
        return circBuffs[buffer]->read();
    }

    /** Non-blocking version of readMessage **/
    bool readMessageNB(size_t buffer, MsgType *result) {
        return circBuffs[buffer]->readNB(result);
    }

    /** Empty the messages in the buffer **/
    void clearBuffer(size_t core) {
       circBuffs[core]->clearBuffer();
    }


private:
    template <typename T>
    std::pair<size_t, T*> reserveSpace(size_t extraSpace = 0)
    {
        size_t space = sizeof(T) + extraSpace;
        if ( ((nextAllocPtr + space) - (uint8_t*)shmPtr) > shmSize )
            return std::make_pair<size_t, T*>(0, NULL);
        T* ptr = (T*)nextAllocPtr;
        nextAllocPtr += space;
        new (ptr) T();  // Call constructor if need be
        return std::make_pair((uint8_t*)ptr - (uint8_t*)shmPtr, ptr);
    }

    size_t static calculateShmemSize(size_t numBuffers, size_t bufferSize)
    {
        long page_size = sysconf(_SC_PAGESIZE);

        /* Count how many pages are needed, at minimum */
        size_t isd = 1 + ((sizeof(InternalSharedData) + (1+numBuffers)*sizeof(size_t)) / page_size);
        size_t buffer = 1+ ((sizeof(CircBuff_t) +
                bufferSize*sizeof(MsgType)) / page_size);
        size_t shdata = 1+ ((sizeof(ShareDataType) + sizeof(InternalSharedData)) / page_size);

        /* Alloc 2 extra pages, just in case */
        return (2 + isd + shdata + numBuffers*buffer) * page_size;
    }

protected:
    /** Pointer to the Shared Data Region */
    ShareDataType *sharedData;

private:
    bool master;
    int fd;
    std::string filename;
    void *shmPtr;
    uint8_t *nextAllocPtr;
    size_t shmSize;
    InternalSharedData *isd;
    std::vector<CircBuff_t* > circBuffs;

};

}
}
}



#endif
