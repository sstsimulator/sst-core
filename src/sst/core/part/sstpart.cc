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


#include <sst_config.h>
#include <sst/core/part/sstpart.h>


using namespace SST;
using namespace SST::Partition;

//std::map<std::string, SSTPartitioner::partitionerAlloc> SSTPartitioner::partitioner_allocs;
//std::map<std::string, std::string> SSTPartitioner::partitioner_descriptions;


SSTPartitioner::SSTPartitioner() {
}

bool
SSTPartitioner::addPartitioner(const std::string name, const SSTPartitioner::partitionerAlloc alloc, const std::string description)
{
    partitioner_allocs()[name] = alloc;
    partitioner_descriptions()[name] = description;
    return true;
}

SSTPartitioner*
SSTPartitioner::getPartitioner(std::string name, RankInfo total_ranks, RankInfo my_rank, int verbosity)
{
    if ( partitioner_allocs().find(name) == partitioner_allocs().end() ) return NULL;
    partitionerAlloc alloc = partitioner_allocs()[name];
    return (*alloc)(total_ranks, my_rank, verbosity);
    
}

std::map<std::string, SSTPartitioner::partitionerAlloc>&
SSTPartitioner::partitioner_allocs()
{
    static std::map<std::string, SSTPartitioner::partitionerAlloc> cache;
    return cache;
}

std::map<std::string, std::string>&
SSTPartitioner::partitioner_descriptions()
{
    static std::map<std::string, std::string> cache;
    return cache;
}
