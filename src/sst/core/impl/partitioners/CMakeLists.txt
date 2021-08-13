# SST-CORE src/sst/core/impl/partitioners CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_library(partitioner OBJECT linpart.cc rrobin.cc selfpart.cc simplepart.cc
                               singlepart.cc)

target_include_directories(partitioner PUBLIC ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(partitioner PUBLIC sst-config-headers)

# Not supporting ZOLTAN with CMake if(ZOLTAN_FOUND)
# target_sources(libpartitioner zoltpart.cc) endif(ZOLTAN_FOUND)

# EOF
