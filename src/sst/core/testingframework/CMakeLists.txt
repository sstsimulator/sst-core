# SST-CORE src/sst/core/testingframework CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
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
