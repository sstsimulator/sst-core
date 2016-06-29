// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_INTERPROCESS_TUNNEL_H
#define SST_CORE_INTERPROCESS_TUNNEL_H 1


#include <tuple>
#include <cstdio>
#include <vector>
#include <string>
#include <errno.h>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include <sst/core/interprocess/circularBuffer.h>
#include <sst/core/output.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_xsi_shared_memory.hpp>


namespace SST {
namespace Core {
namespace Interprocess {

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
        size_t shmSegSize;
        size_t numBuffers;
        size_t offsets[0];  // Actual size:  numBuffers + 2
    };


public:
    /**
     * Construct a new Tunnel for IPC Communications
     * @param region_name Name of the shared-memory region to use.
     * @param numBuffers Number of buffers for which we should tunnel
     * @param bufferSize How large each core's buffer should be
     */
    IPCTunnel(const std::string &region_name, size_t numBuffers,
            size_t bufferSize)
    {
        if ( region_name.length() == 0 ) {
            char *fname = (char*) malloc(sizeof(char) * 1024);
            const char *tmpdir = getenv("TMPDIR");
            if ( !tmpdir ) tmpdir = "/tmp";
            sprintf(fname, "%s/sst_shmem_%u_XXXXXX", tmpdir, getpid());
            fd = mkstemp(fname);
            filename = fname;
            free(fname);
        } else {
            fd = open(region_name.c_str(), O_CREAT,O_RDWR|O_EXCL);
            filename = region_name;
        }

        if ( fd < 0 ) {
            Output::getDefaultObject().fatal(CALL_INFO, -1, "Failed to open IPC region '%s': %s\n",
                    filename.c_str(), strerror(errno));
        }


        shmSize = calculateShmemSize(numBuffers, bufferSize);
        if ( ftruncate(fd, shmSize) ) {
            Output::getDefaultObject().fatal(CALL_INFO, -1, "Resizing shared file '%s' failed: %s\n",
                    filename.c_str(), strerror(errno));
        }
        fsync(fd);

        shmPtr = mmap(NULL, shmSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            Output::getDefaultObject().fatal(CALL_INFO, -1, "mmap failed: %s\n", strerror(errno));
        }
        nextAllocPtr = (uint8_t*)shmPtr;
        memset(shmPtr, '\0', shmSize);

        /* Construct our private buffer first.  Used for our communications */
        size_t offset;
        std::tie(offset, isd) = reserveSpace<InternalSharedData>((1+numBuffers)*sizeof(size_t));
        isd->shmSegSize = shmSize;
        isd->numBuffers = numBuffers;

        /* Construct user's shared-data region */
        std::tie(isd->offsets[0], sharedData) = reserveSpace<ShareDataType>(0);

        /* Construct the circular buffers */
        const size_t cbSize = sizeof(MsgType) * bufferSize;
        for ( size_t c = 0 ; c < isd->numBuffers ; c++ ) {
            CircBuff_t* cPtr = NULL;
            std::tie(isd->offsets[1+c], cPtr) = reserveSpace<CircBuff_t>(cbSize);
            cPtr->setBufferSize(bufferSize);
            circBuffs.push_back(cPtr);
        }

    }

    /**
     * Access an existing Tunnel
     * @param region_name Name of the shared-memory region to access
     */
    IPCTunnel(const std::string &region_name)
    {
        fd = open(region_name.c_str(), O_CREAT,O_RDWR);
        filename = region_name;

        if ( fd < 0 ) {
            Output::getDefaultObject().fatal(CALL_INFO, -1, "Failed to open IPC region '%s': %s\n",
                    filename.c_str(), strerror(errno));
        }

        shmPtr = mmap(NULL, sizeof(InternalSharedData), PROT_READ, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            Output::getDefaultObject().fatal(CALL_INFO, -1, "mmap failed: %s\n", strerror(errno));
        }

        isd = (InternalSharedData*)shmPtr;
        shmSize = isd->shmSegSize;
        munmap(shmPtr, sizeof(InternalSharedData));

        shmPtr = mmap(NULL, shmSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            Output::getDefaultObject().fatal(CALL_INFO, -1, "mmap failed: %s\n", strerror(errno));
        }
        isd = (InternalSharedData*)shmPtr;
        sharedData = (ShareDataType*)((uint8_t*)shmPtr + isd->offsets[0]);

        for ( size_t c = 0 ; c < isd->numBuffers ; c++ ) {
            circBuffs.push_back((CircBuff_t*)((uint8_t*)shmPtr + isd->offsets[c+1]));
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
        if ( shmPtr ) {
            munmap(shmPtr, shmSize);
            shmPtr = NULL;
            shmSize = 0;
        }
        if ( fd >= 0 ) {
            close(fd);
            fd = -1;
        }
        unlink(filename.c_str());
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
