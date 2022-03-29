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

#ifndef SST_CORE_INTERPROCESS_TUNNEL_MMAP_CHILD_PIN3_H
#define SST_CORE_INTERPROCESS_TUNNEL_MMAP_CHILD_PIN3_H

#include "sst/core/interprocess/tunneldef.h"

#include "pin.H"

namespace SST {
namespace Core {
namespace Interprocess {

/** Class supports an IPC tunnel between two or more processes, via an mmap'd file
 * This class attaches to an existing tunnel for a child process using PinCRT
 *
 * @tparam TunnelType Tunnel definition
 */
template <typename TunnelType>
class MMAPChild_Pin3
{

public:
    /** Child-side tunnel manager for an IPC tunnel
     * Opens an existing file and mmaps it using PinCRT
     *
     * @param file_name Name of the shared file to mmap
     */
    MMAPChild_Pin3(const std::string& file_name)
    {
        filename = file_name;
        NATIVE_FD      fd;
        OS_RETURN_CODE retval = OS_OpenFD(filename.c_str(), OS_FILE_OPEN_TYPE_READ | OS_FILE_OPEN_TYPE_WRITE, 0, &fd);

        if ( !OS_RETURN_CODE_IS_SUCCESS(retval) ) {
            // Not using Output because IPC means Output might not be available
            fprintf(
                stderr, "Failed to open file for IPC '%s' (%d): %s\n", filename.c_str(), retval.os_specific_err,
                strerror(retval.os_specific_err));
            exit(1);
        }

        shmPtr = NULL;
        retval = OS_MapFileToMemory(
            NATIVE_PID_CURRENT, OS_PAGE_PROTECTION_TYPE_READ | OS_PAGE_PROTECTION_TYPE_WRITE,
            sizeof(InternalSharedData), OS_MEMORY_FLAGS_SHARED, fd, 0, &shmPtr);

        if ( !OS_RETURN_CODE_IS_SUCCESS(retval) ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "mmap failed (%d): %s\n", retval.os_specific_err, strerror(retval.os_specific_err));
            exit(1);
        }

        // Attach to tunnel to discover actual tunnel size
        tunnel  = new TunnelType(shmPtr);
        shmSize = tunnel->getTunnelSize();

        OS_FreeMemory(NATIVE_PID_CURRENT, shmPtr, sizeof(InternalSharedData));

        // Remap with correct size
        retval = OS_MapFileToMemory(
            NATIVE_PID_CURRENT, OS_PAGE_PROTECTION_TYPE_READ | OS_PAGE_PROTECTION_TYPE_WRITE, shmSize,
            OS_MEMORY_FLAGS_SHARED, fd, 0, &shmPtr);

        if ( !OS_RETURN_CODE_IS_SUCCESS(retval) ) {
            // Not using Output because IPC means Output might not be available
            fprintf(stderr, "mmap failed (%d): %s\n", retval.os_specific_err, strerror(retval.os_specific_err));
            exit(1);
        }

        OS_CloseFD(fd);

        // Finish setup of tunnel with correctly-sized mmap
        tunnel->initialize(shmPtr);
    }

    /** Close file and shutdown tunnel */
    virtual ~MMAPChild_Pin3()
    {
        delete tunnel;
        OS_FreeMemory(NATIVE_PID_CURRENT, shmPtr, shmSize);
    }

    /** return a pointer to the tunnel */
    TunnelType* getTunnel() { return tunnel; }

    /** Return the name of the mmap'd file */
    const std::string& getRegionName(void) const { return filename; }

private:
    void*       shmPtr;
    std::string filename;
    size_t      shmSize;

    TunnelType* tunnel;
};

} // namespace Interprocess
} // namespace Core
} // namespace SST

#endif // SST_CORE_INTERPROCESS_TUNNEL_MMAP_CHILD_PIN3_H
