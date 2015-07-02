// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_OBJECTCOMMS_H
#define SST_CORE_OBJECTCOMMS_H

#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

namespace SST {

#ifdef HAVE_MPI
template <typename dataType>
void broadcast(dataType& data, int root) {
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if ( root == rank ) {
        // Serialize the data
        std::vector<char> buffer;

        boost::iostreams::back_insert_device<std::vector<char> > inserter(buffer);
        boost::iostreams::stream<boost::iostreams::back_insert_device<std::vector<char> > > output_stream(inserter);
        boost::archive::polymorphic_binary_oarchive oa(output_stream, boost::archive::no_header);

        oa << data;
        output_stream.flush();

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
        boost::iostreams::basic_array_source<char> source(buffer,size);
        boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
        boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header );

        ia >> data;
    }
}

template <typename dataType>
void send(int dest, int tag, dataType& data) {
    // Serialize the data
    std::vector<char> buffer;

    boost::iostreams::back_insert_device<std::vector<char> > inserter(buffer);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::vector<char> > > output_stream(inserter);
    boost::archive::polymorphic_binary_oarchive oa(output_stream, boost::archive::no_header);

    oa << data;
    output_stream.flush();

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
    boost::iostreams::basic_array_source<char> source(buffer,size);
    boost::iostreams::stream<boost::iostreams::basic_array_source <char> > input_stream(source);
    boost::archive::polymorphic_binary_iarchive ia(input_stream, boost::archive::no_header );
    
    ia >> data;
}
#endif


} //namespace SST

#endif // SST_CORE_OBJECTCOMMS_H
