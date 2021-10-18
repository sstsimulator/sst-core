# SST-CORE src/sst/core/sync CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_library(sync OBJECT rankSyncParallelSkip.cc rankSyncSerialSkip.cc
                        syncManager.cc syncQueue.cc threadSyncSimpleSkip.cc
                        threadSyncDirectSkip.cc)

target_compile_definitions(sync PRIVATE SST_BUILDING_CORE=1)
target_include_directories(sync PUBLIC ${SST_TOP_SRC_DIR}/src)
target_link_libraries(sync PRIVATE sst-config-headers)

if(MPI_FOUND)
  target_link_libraries(sync PRIVATE MPI::MPI_CXX)
endif(MPI_FOUND)

# EOF
