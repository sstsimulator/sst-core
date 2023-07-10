# ~~~
# SST-CORE src/sst/core/testElements CMake
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

add_library(
  coreTestElement MODULE
  coreTest_Component.cc
  coreTest_ClockerComponent.cc
  coreTest_DistribComponent.cc
  coreTest_RNGComponent.cc
  coreTest_Serialization.cc
  coreTest_StatisticsComponent.cc
  coreTest_Links.cc
  coreTest_MessageGeneratorComponent.cc
  coreTest_SharedObjectComponent.cc
  coreTest_SubComponent.cc
  coreTest_Module.cc
  coreTest_ParamComponent.cc
  coreTest_PerfComponent.cc)

add_subdirectory(message_mesh)

# NEED To double check this
target_include_directories(coreTestElement PRIVATE ${SST_TOP_SRC_DIR}/src)
target_link_libraries(coreTestElement PRIVATE sst-config-headers)
install(TARGETS coreTestElement DESTINATION lib/sstcore)
set_target_properties(coreTestElement PROPERTIES NO_SONAME 1)

if(APPLE)
  target_link_options(coreTestElement PRIVATE
                      "LINKER:-undefined,dynamic_lookup")
endif()

# NEED To figure out how to deal with distributing the README

# EOF
