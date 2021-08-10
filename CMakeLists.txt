# SST-CORE Top-Level CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

cmake_minimum_required(VERSION 3.13)
project(
  sst-core
  VERSION 11.1.0
  DESCRIPTION "SSTCore"
  LANGUAGES C CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
  message(
    STATUS "CMAKE_CXX_STANDARD was not set, defaulting the cmake standard to 11"
  )
else(NOT CMAKE_CXX_STANDARD)
  if(CMAKE_CXX_STANDARD LESS 11)
    message(FATAL_ERROR "We require the c++ standard to be at least 11")
  endif(CMAKE_CXX_STANDARD LESS 11)
endif(NOT CMAKE_CXX_STANDARD)

# ------------------------------------------------------------------------
# -- EXTERNAL CMAKE SCRIPTS
# ------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(Python 3.5 REQUIRED COMPONENTS Interpreter Development)

if(SST_ENABLE_LIBZ)
  find_package(ZLIB REQUIRED)
endif(SST_ENABLE_LIBZ)

if(SST_ENABLE_HDF5)
  find_package(HDF5 REQUIRED)
endif(SST_ENABLE_HDF5)

if(NOT SST_DISABLE_MPI)
  find_package(MPI REQUIRED)
endif(NOT SST_DISABLE_MPI)

# sst sets all the variables needed for our *.in files
include(sst REQUIRED)
add_subdirectory(cmake_configure_files)

# ------------------------------------------------------------------------
# -- COMPILER OPTIONS
# ------------------------------------------------------------------------
# TODO figure out if we can not use this this way
set(CMAKE_ELEMENT_CXXFLAGS
    "-std=c++1y -D__STDC_FORMAT_MACROS -fPIC -I${CMAKE_INSTALL_PREFIX}/include")
set(CMAKE_ELEMENT_LDFLAGS
    "-shared -fno-common -Wl,-undefined -Wl,dynamic_lookup")

set(SST_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SST_LINK_LIBS "")
set(SST_LIBS "")

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
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SST_DEBUG_EVENT_TRACKING__=1")
  endif()
else()
  message(STATUS "SST: Disabling Event Tracking")
endif()

if(SST_ENABLE_DEBUG_OUTPUT)
  message(STATUS "SST: Enabling Debug Output")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SST_DEBUG_OUTPUT__=1")
else()
  message(STATUS "SST: Disabling Debug Output")
endif()

# TODO ENABLE_PROFILE_BUILD doesn't appear in the project
if(SST_ENABLE_PROFILE_BUILD)
  message(STATUS "SST: Enabling Core Profiling")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSST_ENABLE_PROFILE_BUILD=1")
else()
  message(STATUS "SST: Disabling Core Profiling")
endif()

# ------------------------------------------------------------------------
# -- FIND PACKAGES
# ------------------------------------------------------------------------

message(
  STATUS
    "Found Python ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}.${Python_VERSION_PATCH}"
)
set(Python_CONFIG "${Python_EXECUTABLE}-config")
set(SST_PYTHON_LIB "-lpython${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
# set(SST_LIB "${SST_LIB} ${SST_PYTHON_LIB}")

execute_process(
  COMMAND bash -c "${Python_CONFIG} --includes"
  OUTPUT_VARIABLE Python_CPPFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND bash -c "${Python_CONFIG} --ldflags"
  OUTPUT_VARIABLE Python_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND bash -c "${Python_CONFIG} --libs"
  OUTPUT_VARIABLE Python_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE)
# set(SST_LIB "${SST_LIB} ${Python_LIBS}")

message(STATUS "Python Intpreter found at ${Python_EXECUTABLE}")
message(STATUS "Python Config found at ${Python_CONFIG}")
message(STATUS "Python Libraries found at ${Python_LIBRARY_DIRS}")
message(STATUS "Python Headers found at ${Python_INCLUDE_DIRS}")
message(STATUS "Python link options are ${Python_LINK_OPTIONS}")
message(STATUS "Python CPPFLAGS are: ${Python_CPPFLAGS}")
message(STATUS "Python LDFLAGS are: ${Python_LDFLAGS}")
message(STATUS "Python LIBS are: ${Python_LIBS}")

# -- LibZ
if(SST_ENABLE_LIBZ)
  if(ZLIB_FOUND)
    message(STATUS "LibZ Headers found at ${ZLIB_INCLUDE_DIRS}")
    message(STATUS "LibZ Libraries found at ${ZLIB_LIBRARIES}")
    # set(SST_LINK_LIBS "${SST_LINK_LIBS} ${ZLIB_LIBRARIES}") set(SST_ZLIB_MACRO
    # "#define HAVE_LIBZ 1") set(SST_LIB "${SST_LIB} ${ZLIB_LIBRARIES}")
  else()
    message(FATAL_ERROR "LibZ not found")
  endif()
else()
  message(STATUS "LibZ support disabled")
  # set(ZLIB_INCLUDE_DIRS "") set(ZLIB_LIBRARIES "") set(SST_ZLIB_MACRO "/*
  # #undef HAVE_LIBZ */")
endif()

# -- HDF5
if(SST_ENABLE_HDF5)
  if(HDF5_FOUND)
    message(STATUS "HDF5 Headers found at ${HDF5_INCLUDE_DIRS}")
    message(STATUS "HDF5 Libraries found at ${HDF5_LIBRARIES}")
    message(STATUS "HDF5 Compiler definitions set to ${HDF5_DEFINITIONS}")
    # set(SST_LINK_LIBS "${SST_LINK_LIBS} ${HDF5_LIBRARIES}") set(SST_HDF5_MACRO
    # "#define HAVE_HDF5 1") set(SST_LIB "${SST_LIB} ${HDF5_LIBRARIES}")
  else()
    message(FATAL_ERROR "HDF5 not found")
  endif()
else()
  # set(SST_HDF5_MACRO "/* #undef HAVE_HDF5 */") set(HDF5_INCLUDE_DIRS "")
  # set(HDF5_LINK_LIBS "")
endif()

# -- MPI
if(NOT SST_DISABLE_MPI)
  if(MPI_FOUND)
    message(STATUS "MPI C Compiler found at ${MPI_C_COMPILER}")
    message(STATUS "MPI CXX Compiler found at ${MPI_CXX_COMPILER}")
    message(STATUS "MPI Headers found at ${MPI_CXX_INCLUDE_DIRS}")
    message(STATUS "MPI Libraries found at ${MPI_CXX_LINK_FLAGS}")
    message(STATUS "MPI Link libraries set to ${MPI_CXX_LIBRARIES}")
    # set(CMAKE_C_COMPILER "${MPI_C_COMPILER}") set(CMAKE_CXX_COMPILER
    # "${MPI_CXX_COMPILER}") set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}
    # -DSST_CONFIG_HAVE_MPI=1") set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}
    # -DSST_CONFIG_HAVE_MPI=1") set(HAVE_MPI "1") set(SST_MPI_MACRO "#define
    # SST_CONFIG_HAVE_MPI ${HAVE_MPI}")
  else()
    message(FATAL_ERROR "MPI not found")
  endif()
else()
  # set(MPI_C_COMPILER "${CMAKE_C_COMPILER}") set(MPI_CXX_COMPILER
  # "${CMAKE_CXX_COMPILER}") set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") set(HAVE_MPI "0")
  # set(SST_MPI_MACRO "/* #undef SST_CONFIG_HAVE_MPI */")
endif()

# -- Doxygen
add_subdirectory(doc)


# ------------------------------------------------------------------------
# -- SST-CORE LINKER OPTIONS
# ------------------------------------------------------------------------
set(SST_SIM_LDFLAGS
    "${Python_LDFLAGS} ${SST_PYTHON_LIB} -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions -export-dynamic ${SST_LTDL_LIBS} ${SST_LINK_LIBS}"
)
set(SST_INFO_LDFLAGS
    "${Python_LDFLAGS} ${SST_PYTHON_LIB} -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions -export-dynamic ${SST_LTDL_LIBS} ${SST_LINK_LIBS}"
)

set(SST_LIB "${SST_LIB} ${SST_LTDL_LIBS}")

if(NOT SST_COMPILE_OSX)
  set(SST_SIM_LDFLAGS "${SST_SIM_LDFLAGS} -lrt")
  set(SST_INFO_LDFLAGS "${SST_INFO_LDFLAGS} -lrt")
  set(SST_LIB "${SST_LIB} -lrt")
endif()

string(STRIP ${SST_LIB} SST_LIB)

message(STATUS "SST_INCLUDE_DIRS: ${SST_INCLUDE_DIRS}")
message(STATUS "SST SIM LDFLAGS: ${SST_SIM_LDFLAGS}")
message(STATUS "SST-INFO LDFLAGS: ${SST_INFO_LDFLAGS}")
message(STATUS "SST LINK LIBRARIES: ${SST_LIB}")

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
add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(share)

# EOF
