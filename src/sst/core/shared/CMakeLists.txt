# SST-CORE src/sst/core/shared CMake
#
# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
#

add_library(shared OBJECT sharedObject.cc)
target_include_directories(shared PRIVATE ${SST_TOP_SRC_DIR}/src)
target_link_libraries(shared PRIVATE sst-config-headers)

if(MPI_FOUND)
  target_link_libraries(shared PRIVATE MPI::MPI_CXX)
endif()

set(SSTSharedHeaders sharedArray.h sharedMap.h sharedObject.h sharedSet.h)

install(FILES ${SSTSharedHeaders} DESTINATION "include/sst/core/shared")

# EOF
