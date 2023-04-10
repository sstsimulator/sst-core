# SST-CORE src/sst/core/model/json CMake
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
