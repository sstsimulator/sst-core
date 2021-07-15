# SST-CORE tests/ CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

if(BUILD_ALL_TESTING)
  add_test(NAME sst-test-core
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${Python_EXECUTABLE} ${CMAKE_INSTALL_PREFIX}/bin/sst-test-core )
  set_test_properties( sst-test-core PROPERTIES PASS_REGULAR_EXPRESSION "TESTING PASSED")
endif()

# EOF
