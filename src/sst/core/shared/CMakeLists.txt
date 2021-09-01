# SST-CORE src/sst/core/shared CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_library(shared OBJECT sharedObject.cc)
target_include_directories(shared PRIVATE ${sst-core_SOURCE_DIR}/../src)
target_link_libraries(shared PRIVATE sst-config-headers)

if(MPI_FOUND)
  target_link_libraries(shared PRIVATE MPI::MPI_CXX)
endif()

set(SSTSharedHeaders sharedArray.h sharedMap.h sharedObject.h sharedSet.h)

install(FILES ${SSTSharedHeaders} DESTINATION "include/sst/core/shared")

# EOF
