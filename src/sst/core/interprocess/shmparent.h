// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_INTERPROCESS_TUNNEL_SHM_PARENT_H
#define SST_CORE_INTERPROCESS_TUNNEL_SHM_PARENT_H 1


#include <fcntl.h>
#include <cstdio>
#include <string>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>

namespace SST {
namespace Core {
namespace Interprocess {

/** Class supports an IPC tunnel between two or more processes via posix shared memory
 * This class creates the tunnel for the parent/master process
 *
 * @tparam TunnelType Tunnel definition
 */
template<typename TunnelType>
class SHMParent {

public:
    /** Parent/master manager for an IPC tunnel
     * Creates a shared memory region and initializes a
     * TunnelType data strucgture in the region
     *
     * @param comp_id Component ID of owner
     * @param numBuffers Number of buffers for which we should tunnel
     * @param bufferSize How large each core's buffer should be
     * @param expectedChildren How many child processes will connect to the tunnel
     */
    SHMParent(uint32_t comp_id, size_t numBuffers, size_t bufferSize, uint32_t expectedChildren = 1) : shmPtr(nullptr), fd(-1)
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

        tunnel = new TunnelType(numBuffers, bufferSize, expectedChildren);
        shmSize = tunnel->getTunnelSize();

        if ( ftruncate(fd, shmSize) ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "Resizing shared file '%s' failed: %s\n", filename.c_str(), strerror(errno));
            exit(1);
        }

        shmPtr = mmap(nullptr, shmSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "mmap failed: %s\n", strerror(errno));
            exit(1);
        }
        memset(shmPtr, '\0', shmSize);
        tunnel->initialize(shmPtr);
    }

    /** Destructor */
    virtual ~SHMParent()
    {
        delete tunnel;
        if ( shmPtr ) {
            munmap(shmPtr, shmSize);
            shmPtr = nullptr;
            shmSize = 0;
        }
        if ( fd >= 0 ) {
            close(fd);
            fd = -1;
        }
    }

    /** returns name of the mmap'd region */
    const std::string& getRegionName(void) const { return filename; }

    /** return the created tunnel pointer */
    TunnelType* getTunnel() { return tunnel; }

private:
    void *shmPtr;
    int fd;

    std::string filename;
    size_t shmSize;

    TunnelType* tunnel;
};

}
}
}



#endif
