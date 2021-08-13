# SST-CORE src/sst/core/testElements CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_library(
  coreTestElement MODULE
  coreTest_Component.cc
  coreTest_ClockerComponent.cc
  coreTest_DistribComponent.cc
  coreTest_RNGComponent.cc
  coreTest_StatisticsComponent.cc
  coreTest_Links.cc
  coreTest_MessageGeneratorComponent.cc
  coreTest_SharedObjectComponent.cc
  coreTest_SubComponent.cc
  coreTest_Module.cc
  coreTest_ParamComponent.cc)

# NEED To double check this
target_include_directories(coreTestElement PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(coreTestElement PRIVATE sst-config-headers)
install(TARGETS coreTestElement DESTINATION lib/sstcore)
set_target_properties(coreTestElement PROPERTIES NO_SONAME 1)

# NEED To figure out how to deal with distributing the README

# EOF
