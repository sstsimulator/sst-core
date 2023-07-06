# ~~~
# SST-CORE src/sst/core/sync CMake
#
# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
# ~~~
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
