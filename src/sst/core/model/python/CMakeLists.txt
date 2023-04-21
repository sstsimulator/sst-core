# SST-CORE src/sst/core/model/python CMake
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
  modelpython OBJECT
  pymodel.cc pymodel_link.cc pymodel_comp.cc pymodel_unitalgebra.cc
  pymodel_stat.cc pymodel_statgroup.cc)

target_include_directories(modelpython PUBLIC ${SST_TOP_SRC_DIR}/src/)
target_link_libraries(modelpython PRIVATE Python::Python sst-config-headers)

if(MPI_FOUND)
  target_link_libraries(modelpython PRIVATE MPI::MPI_CXX)
endif(MPI_FOUND)

install(FILES pymacros.h DESTINATION "include/sst/core/model/python")

# EOF
