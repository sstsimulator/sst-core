# SST-CORE tests/ CMake
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

# TODO probably don't want to run tests in the source dir
option(SST_ENABLE_TESTS "add option to run make test" ON)
if(SST_ENABLE_TESTS)
  add_test(
    NAME sst-test-core
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${Python_EXECUTABLE} ${CMAKE_BINARY_DIR}/src/sst/core/testingframework/sst-test-core)
  set_tests_properties(sst-test-core PROPERTIES PASS_REGULAR_EXPRESSION
                                                "TESTING PASSED")
endif()

# EOF
