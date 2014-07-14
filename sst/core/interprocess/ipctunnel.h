#ifndef SST_CORE_INTERPROCESS_TUNNEL_H
#define SST_CORE_INTERPROCESS_TUNNEL_H 1


#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>

#include <sst/core/interprocess/circularBuffer.h>

#include <boost/interprocess/managed_shared_memory.hpp>


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

    typedef SST::Core::Interprocess::CircularBuffer<
            MsgType,
            boost::interprocess::allocator<MsgType,
                boost::interprocess::managed_shared_memory::segment_manager> >
        CircBuff_t;

    struct InternalSharedData {
        size_t numBuffers;
    };

public:
    /**
     * Construct a new Tunnel for IPC Communications
     * @param region_name Name of the shared-memory region to use.
     * @param numBuffers Number of buffers for which we should tunnel
     * @param bufferSize How large each core's buffer should be
     */
    IPCTunnel(const std::string &region_name, size_t numBuffers,
            size_t bufferSize) :
        regionName(region_name)
    {
        /* Remove any lingering mappings */
        boost::interprocess::shared_memory_object::remove(regionName.c_str());

        shm = boost::interprocess::managed_shared_memory(
                boost::interprocess::create_only,
                regionName.c_str(), calculateShmemSize(numBuffers, bufferSize));

        /* Construct our private buffer first.  Used for our communications */
        isd = shm.construct<InternalSharedData>("InternalShared")();
        isd->numBuffers = numBuffers;

        /* Construct user's shared-data region */
        sharedData = shm.construct<ShareDataType>("Shared Data")();

        /* Construct the circular buffers */
        char bufName[1024];
        for ( size_t c = 0 ; c < numBuffers ; c++ ) {
            sprintf(bufName, "buffer%zu", c);
            circBuffs.push_back(shm.construct<CircBuff_t>(bufName)(
                        bufferSize, shm.get_segment_manager()));
        }

    }

    /**
     * Access an existing Tunnel
     * @param region_name Name of the shared-memory region to access
     */
    IPCTunnel(const std::string &region_name) :
        regionName(region_name),
        shm(boost::interprocess::managed_shared_memory(
                    boost::interprocess::open_only, regionName.c_str()))
    {
        isd = shm.find<InternalSharedData>("InternalShared").first;
        sharedData = shm.find<ShareDataType>("Shared Data").first;

        char bufName[1024];
        for ( size_t c = 0 ; c < isd->numBuffers ; c++ ) {
            sprintf(bufName, "buffer%zu", c);
            circBuffs.push_back(shm.find<CircBuff_t>(bufName).first);
        }
    }


    /**
     * Destructor
     */
    virtual ~IPCTunnel()
    {
        boost::interprocess::shared_memory_object::remove(regionName.c_str());
    }


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
    size_t calculateShmemSize(size_t numBuffers, size_t bufferSize) const
    {
        long page_size = sysconf(_SC_PAGESIZE);

        /* Count how many pages are needed, at minimum */
        size_t buffer = 1+ ((sizeof(CircBuff_t) +
                bufferSize*sizeof(MsgType)) / page_size);
        size_t shdata = 1+ ((sizeof(ShareDataType) + sizeof(InternalSharedData)) / page_size);

        /* Alloc 2 extra pages, just in case */
        return (2 + shdata + numBuffers*buffer) * page_size;

    }

protected:
    /** Pointer to the Shared Data Region */
    ShareDataType *sharedData;

private:
    std::string regionName;
    boost::interprocess::managed_shared_memory shm;
    InternalSharedData *isd;
    std::vector<CircBuff_t* > circBuffs;

};

}
}
}



#endif
