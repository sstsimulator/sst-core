# ~~~
# SST-CORE src/sst/core/testingframework CMake
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

configure_file(sst-test-core-cmake.in sst-test-core)
configure_file(sst-test-elements-cmake.in sst-test-elements)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/sst-test-core
        ${CMAKE_CURRENT_BINARY_DIR}/sst-test-elements sst_test_engine_loader.py
  DESTINATION bin
  PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)

install(
  FILES test_engine.py
        sst_unittest_parameterized.py
        sst_unittest.py
        test_engine_globals.py
        test_engine_junit.py
        test_engine.py
        test_engine_support.py
        test_engine_unittest.py
        sst_unittest_support.py
  DESTINATION libexec)

# EOF
