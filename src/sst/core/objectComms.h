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


#ifndef SST_CORE_OBJECTCOMMS_H
#define SST_CORE_OBJECTCOMMS_H

#include <sst/core/warnmacros.h>
#ifdef SST_CONFIG_HAVE_MPI
DISABLE_WARN_MISSING_OVERRIDE
#include <mpi.h>
REENABLE_WARNING
#endif

#include <sst/core/serialization/serializer.h>

#include <typeinfo>

namespace SST {

namespace Comms {


template <typename dataType>
std::vector<char> serialize(dataType &data)
{
    SST::Core::Serialization::serializer ser;

    ser.start_sizing();
    ser & data;
    
    int size = ser.size();
    
    std::vector<char> buffer;
    buffer.resize(size);

    ser.start_packing(buffer.data(),size);
    ser & data;

    return buffer;    
}

// template <typename dataType>
// std::vector<char> serialize(dataType* data)
// {
//     SST::Core::Serialization::serializer ser;

//     std::cout << typeid(data).name() << std::endl;
//     ser.start_sizing();
//     ser & data;

//     ser.start_sizing();
//     ser & data;
    
//     int size = ser.size();
//     std::cout << "serialize size = " << size << std::endl;
    
//     std::vector<char> buffer;
//     buffer.resize(size);

//     ser.start_packing(buffer.data(),size);
//     ser & data;

//     return buffer;    
// }


template <typename dataType>
dataType* deserialize(std::vector<char> &buffer)
{
    dataType *tgt = NULL;

    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer.data(), buffer.size());
    ser & tgt;
    
    return tgt;
}

template <typename dataType>
void deserialize(std::vector<char> &buffer, dataType &tgt)
{ 
    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer.data(), buffer.size());
    ser & tgt;
}

template <typename dataType>
void deserialize(char *buffer, int blen, dataType &tgt)
{
    SST::Core::Serialization::serializer ser;

    ser.start_unpacking(buffer, blen);
    ser & tgt;
}

#ifdef SST_CONFIG_HAVE_MPI
template <typename dataType>
void broadcast(dataType& data, int root) {
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if ( root == rank ) {
        // Serialize the data
        std::vector<char> buffer = Comms::serialize(data);

        // Now broadcast the size of the data
        int size = buffer.size();
        MPI_Bcast(&size, 1, MPI_INT, root, MPI_COMM_WORLD);

        // Now broadcast the data
        MPI_Bcast(buffer.data(), buffer.size(), MPI_BYTE, root, MPI_COMM_WORLD);
    }
    else {
        // Get the size of the broadcast
        int size = 0;
        MPI_Bcast(&size, 1, MPI_INT, root, MPI_COMM_WORLD);

        // Now get the data
        char* buffer = new char[size];
        MPI_Bcast(buffer, size, MPI_BYTE, root, MPI_COMM_WORLD);

        // Now deserialize data
        Comms::deserialize(buffer, size, data);
    }
}

template <typename dataType>
void send(int dest, int tag, dataType& data) {
    // Serialize the data
    std::vector<char> buffer = Comms::serialize<dataType>(data);

    // Now send the data.  Send size first, then payload
    // std::cout<< sizeof(buffer.size()) << std::endl;
    int64_t size = buffer.size();
    MPI_Send(&size, 1, MPI_INT64_T, dest, tag, MPI_COMM_WORLD);

    int32_t fragment_size = 1000000000;
    int64_t offset = 0;

    while ( size >= fragment_size ) {
        MPI_Send(buffer.data() + offset, fragment_size, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
        size -= fragment_size;
        offset += fragment_size;
    }
    MPI_Send(buffer.data() + offset, size, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
}

template <typename dataType>
void recv(int src, int tag, dataType& data) {
    // Get the size of the broadcast
    int64_t size = 0;
    MPI_Status status;
    MPI_Recv(&size, 1, MPI_INT64_T, src, tag, MPI_COMM_WORLD, &status);

    // Now get the data
    char* buffer = new char[size];
    int64_t offset = 0;
    int32_t fragment_size = 1000000000;
    int64_t rem_size = size;

    while ( rem_size >= fragment_size ) {
        MPI_Recv(buffer + offset, fragment_size, MPI_BYTE, src, tag, MPI_COMM_WORLD, &status);
        rem_size -= fragment_size;
        offset += fragment_size;
    }
    MPI_Recv(buffer + offset, rem_size, MPI_BYTE, src, tag, MPI_COMM_WORLD, &status);


    // Now deserialize data
    Comms::deserialize(buffer, size, data);
}


template <typename dataType>
void all_gather(dataType& data, std::vector<dataType> &out_data) {
    int rank = 0, world = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world);

    // Serialize the data
    std::vector<char> buffer = Comms::serialize(data);


    size_t sendSize = buffer.size();
    int allSizes[world];
    int displ[world];

    memset(allSizes, '\0', world * sizeof(int));
    memset(displ, '\0', world * sizeof(int));

    MPI_Allgather(&sendSize, sizeof(int), MPI_BYTE,
            &allSizes, sizeof(int), MPI_BYTE, MPI_COMM_WORLD);

    int totalBuf = 0;
    for ( int i = 0 ; i < world ; i++ ) {
        totalBuf += allSizes[i];
        if ( i > 0 )
            displ[i] = displ[i-1] + allSizes[i-1];
    }

    char *bigBuff = new char[totalBuf];

    MPI_Allgatherv(buffer.data(), buffer.size(), MPI_BYTE,
            bigBuff, allSizes, displ, MPI_BYTE, MPI_COMM_WORLD);

    out_data.resize(world);
    for ( int i = 0 ; i < world ; i++ ) {
        Comms::deserialize(&bigBuff[displ[i]], allSizes[i], out_data[i]);
    }

    delete [] bigBuff;

}




#endif

}

} //namespace SST

#endif // SST_CORE_OBJECTCOMMS_H
