# SST-CORE share/ CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

# TODO Remove this set set(prefix ${CMAKE_INSTALL_PREFIX})

message(
  STATUS "SST: PREPROCESSING ${CMAKE_CURRENT_SOURCE_DIR}/SSTConfig.cmake.in")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SSTConfig.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/SSTConfig.cmake)

install(
  DIRECTORY "."
  DESTINATION "lib/cmake/SST"
  FILES_MATCHING
  PATTERN "*.cmake")

# EOF
