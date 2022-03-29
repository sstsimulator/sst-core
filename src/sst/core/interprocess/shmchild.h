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

#ifndef SST_CORE_INTERPROCESS_TUNNEL_SHM_CHILD_H
#define SST_CORE_INTERPROCESS_TUNNEL_SHM_CHILD_H

#include "sst/core/interprocess/tunneldef.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace SST {
namespace Core {
namespace Interprocess {

/** Class supports an IPC tunnel between two or more processes, via posix shared memory
 * This class attaches to an existing tunnel for a child process
 *
 * @tparam TunnelType Tunnel definition
 */
template <typename TunnelType>
class SHMChild
{

public:
    /** Child-side tunnel manager for an IPC tunnel
     * Accesses an existing Tunnel using shared memory
     *
     * @param region_name Name of the shared-memory region to access
     */
    SHMChild(const std::string& region_name) : shmPtr(nullptr), fd(-1)
    {
        fd       = shm_open(region_name.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
        filename = region_name;

        if ( fd < 0 ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "Failed to open IPC region '%s': %s\n", filename.c_str(), strerror(errno));
            exit(1);
        }

        shmPtr = mmap(nullptr, sizeof(InternalSharedData), PROT_READ, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "mmap 0 failed: %s\n", strerror(errno));
            exit(1);
        }

        tunnel  = new TunnelType(shmPtr);
        shmSize = tunnel->getTunnelSize();

        munmap(shmPtr, sizeof(InternalSharedData));

        shmPtr = mmap(nullptr, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if ( shmPtr == MAP_FAILED ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "mmap 1 failed: %s\n", strerror(errno));
            exit(1);
        }
        uint32_t childnum = tunnel->initialize(shmPtr);
        if ( childnum == 0 ) { shm_unlink(filename.c_str()); }
    }

    /** Destructor */
    virtual ~SHMChild()
    {
        delete tunnel;
        if ( shmPtr ) {
            munmap(shmPtr, shmSize);
            shmPtr  = nullptr;
            shmSize = 0;
        }
        if ( fd >= 0 ) {
            close(fd);
            fd = -1;
        }
    }

    /** return a pointer to the tunnel */
    TunnelType* getTunnel() { return tunnel; }

    /** return the name of the shared memory region */
    const std::string& getRegionName(void) const { return filename; }

private:
    void* shmPtr;
    int   fd;

    std::string filename;
    size_t      shmSize;

    TunnelType* tunnel;
};

} // namespace Interprocess
} // namespace Core
} // namespace SST

#endif // SST_CORE_INTERPROCESS_TUNNEL_SHM_CHILD_H
