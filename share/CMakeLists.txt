# ~~~
# SST-CORE share/ CMake
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
