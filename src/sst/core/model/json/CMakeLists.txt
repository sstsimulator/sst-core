# SST-CORE src/sst/core/model/json CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_library(
  modeljson OBJECT
  jsonmodel.cc)

target_include_directories(modeljson PUBLIC ${SST_TOP_SRC_DIR}/src/)
target_link_libraries(modeljson PRIVATE sst-config-headers nlohjson)

if(MPI_FOUND)
  target_link_libraries(modeljson PRIVATE MPI::MPI_CXX)
endif(MPI_FOUND)

# EOF
