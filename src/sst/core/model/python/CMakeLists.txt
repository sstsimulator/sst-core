# SST-CORE src/sst/core/model/python CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_library(
  modelpython OBJECT
  pymodel.cc pymodel_link.cc pymodel_comp.cc pymodel_unitalgebra.cc
  pymodel_stat.cc pymodel_statgroup.cc)


target_include_directories(modelpython PUBLIC ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(modelpython PRIVATE Python::Python sst-config-headers)

if(MPI_FOUND)
  target_link_libraries(modelpython PRIVATE MPI::MPI_C)
endif(MPI_FOUND)

install(FILES pymacros.h DESTINATION "include/sst/core/model/python")

# EOF
