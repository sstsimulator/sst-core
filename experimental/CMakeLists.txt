# ~~~
# SST-CORE Top-Level CMake
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

cmake_minimum_required(VERSION 3.16)
project(
  sst-core
  VERSION 13.1.0
  DESCRIPTION "SSTCore"
  LANGUAGES C CXX)

set(SST_TOP_SRC_DIR "${sst-core_SOURCE_DIR}/..")
include(cmake/PreventInSourceBuilds.cmake)

# cmake-lint: disable=C0103
set(_REQUIRED_CXX_STANDARD 17)
# cmake-lint: disable=C0301
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD ${_REQUIRED_CXX_STANDARD})
  message(
    STATUS
      "CMAKE_CXX_STANDARD was not set, defaulting the cmake standard to ${_REQUIRED_CXX_STANDARD}"
  )
else()
  if(CMAKE_CXX_STANDARD LESS ${_REQUIRED_CXX_STANDARD})
    message(
      FATAL_ERROR
        "We require the c++ standard to be at least ${_REQUIRED_CXX_STANDARD}")
  endif()
endif()
unset(_REQUIRED_CXX_STANDARD)

# ------------------------------------------------------------------------
# -- EXTERNAL CMAKE SCRIPTS
# ------------------------------------------------------------------------
# Don't change this path to SST_TOP_SRC_DIR cmake lives in experimental for now.
list(APPEND CMAKE_MODULE_PATH ${sst-core_SOURCE_DIR}/cmake)

find_package(Python 3.6 REQUIRED COMPONENTS Interpreter Development)
find_package(Threads)

option(SST_DISABLE_ZLIB "Use zlib compression library" OFF)
if(NOT SST_DISABLE_ZLIB)
  find_package(ZLIB)
endif()

option(SST_ENABLE_HDF5 "Use HDF5 library" OFF)
if(SST_ENABLE_HDF5)
  find_package(HDF5 REQUIRED)
endif()

option(SST_DISABLE_MPI "Compile without MPI" OFF)
if(NOT SST_DISABLE_MPI)
  find_package(MPI REQUIRED)
endif()

# sst sets all the variables needed for our *.in files
include(sst REQUIRED)
# Don't change this path to SST_TOP_SRC_DIR this lives in experimental for now.
add_subdirectory(${sst-core_SOURCE_DIR}/cmake_configure_files)

# ------------------------------------------------------------------------
# -- COMPILER OPTIONS
# ------------------------------------------------------------------------
option(SST_DISABLE_MEM_POOLS "Compile without using memory pools" OFF)
if(SST_DISABLE_MEM_POOLS)
  message(STATUS "SST: Disabling Mem Pools")
else()
  message(STATUS "SST: Enabling Mem Pools")
  set(USE_MEMPOOL ON)
endif()

if(SST_ENABLE_EVENT_TRACKING)
  if(SST_DISABLE_MEM_POOLS)
    message(FATAL_ERROR "SST: Mem Pools must be enabled for Event Tracking")
  else()
    message(STATUS "SST: Enabling Event Tracking")
  endif()
else()
  message(STATUS "SST: Disabling Event Tracking")
endif()

if(SST_ENABLE_DEBUG_OUTPUT)
  message(STATUS "SST: Enabling Debug Output")
else()
  message(STATUS "SST: Disabling Debug Output")
endif()

# ------------------------------------------------------------------------
# -- FIND PACKAGES
# ------------------------------------------------------------------------

# -- Doxygen
add_subdirectory(${SST_TOP_SRC_DIR}/doc ${sst-core_BINARY_DIR}/doc)

# ------------------------------------------------------------------------
# -- TESTING
# ------------------------------------------------------------------------
enable_testing()

# ------------------------------------------------------------------------
# -- PACKAGING
# ------------------------------------------------------------------------
if(SST_BUILD_RPM)
  set(CPACK_PACKAGE_VERSION ${VERSION})
  set(CPACK_GENERATOR "RPM")
  set(CPACK_PACKAGE_NAME "sst")
  set(CPACK_PACKAGE_CONTACT "sst@sandia.gov")
  set(CPACK_PACKAGE_VENDOR "Sandia, LLC (NTESS)")
  set(CPACK_RPM_PACKAGE_LICENSE "See LICENSE")
  set(CPACK_RPM_PACKAGE_RELOCATABLE ON)
  set(CPACK_RPM_PACKAGE_REQUIRES "devtoolset-8")
  set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
  set(CPACK_PACKAGE_FILE_NAME
      "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}"
  )
  include(CPack)
endif()

if(SST_BUILD_DEB)
  set(CPACK_PACKAGE_VERSION ${VERSION})
  set(CPACK_GENERATOR "DEB")
  set(CPACK_PACKAGE_NAME "sst")
  set(CPACK_PACKAGE_CONTACT "sst@sandia.gov")
  set(CPACK_PACKAGE_VENDOR "Sandia, LLC (NTESS)")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "build-essential")
  include(CPack)
endif()

if(SST_BUILD_TGZ)
  set(CPACK_PACKAGE_VERSION ${VERSION})
  set(CPACK_GENERATOR "TGZ")
  set(CPACK_PACKAGE_NAME "sst")
  set(CPACK_PACKAGE_RELEASE 1)
  set(CPACK_PACKAGE_CONTACT "sst@sandia.gov")
  set(CPACK_PACKAGE_VENDOR "Sandia, LLC (NTESS)")
  set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
  set(CPACK_PACKAGE_FILE_NAME
      "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}"
  )
  include(CPack)
endif()

# ------------------------------------------------------------------------
# -- SUBDIRECTORIES
# ------------------------------------------------------------------------
add_subdirectory(${SST_TOP_SRC_DIR}/external ${sst-core_BINARY_DIR}/external)
add_subdirectory(${SST_TOP_SRC_DIR}/src ${sst-core_BINARY_DIR}/src)
add_subdirectory(${SST_TOP_SRC_DIR}/tests ${sst-core_BINARY_DIR}/tests)
add_subdirectory(${SST_TOP_SRC_DIR}/share ${sst-core_BINARY_DIR}/share)

# EOF
