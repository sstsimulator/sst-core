# SST-CORE Top-Level CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

cmake_minimum_required(VERSION 3.13)
project(sst-core)
set(VERSION "11.1.0")
#------------------------------------------------------------------------
#-- EXTERNAL CMAKE SCRIPTS
#------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(sst REQUIRED)

#------------------------------------------------------------------------
#-- COMPILER OPTIONS
#------------------------------------------------------------------------
 set(CMAKE_C_FLAGS "-DHAVE_CONFIG_H -g -O2 -Wall -Wextra ${CMAKE_C_FLAGS}")
 set(CMAKE_CXX_FLAGS "-DHAVE_CONFIG_H -g -O2 -std=c++1y -D__STDC_FORMAT_MACROS -Wall -Wextra ${CMAKE_CXX_FLAGS}")
 set(CMAKE_ELEMENT_CXXFLAGS "-std=c++1y -D__STDC_FORMAT_MACROS -fPIC -DHAVE_CONFIG_H -I${CMAKE_INSTALL_PREFIX}/include")
 set(CMAKE_ELEMENT_LDFLAGS "-shared -fno-common -Wl,-undefined -Wl,dynamic_lookup")

set(SST_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SST_LINK_LIBS "")
set(SST_LIBS "")

if( SST_DISABLE_MEM_POOLS )
  message(STATUS "SST: Disabling Mem Pools")
else()
  message(STATUS "SST: Enabling Mem Pools")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_MEMPOOL=1")
endif()

if( SST_ENABLE_EVENT_TRACKING )
  if( SST_DISABLE_MEM_POOLS )
    message(FATAL_ERROR "SST: Mem Pools must be enabled for Event Tracking")
  else()
    message(STATUS "SST: Enabling Event Tracking")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SST_DEBUG_EVENT_TRACKING__=1")
  endif()
else()
  message(STATUS "SST: Disabling Event Tracking")
endif()

if( SST_ENABLE_DEBUG )
  message(STATUS "SST: Enabling Debug Output")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SST_DEBUG_OUTPUT__=1")
else()
  message(STATUS "SST: Disabling Debug Output")
endif()

if( SST_ENABLE_PROFILE_BUILD )
  message(STATUS "SST: Enabling Core Profiling")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSST_ENABLE_PROFILE_BUILD=1")
else()
  message(STATUS "SST: Disabling Core Profiling")
endif()

#------------------------------------------------------------------------
#-- FIND PACKAGES
#------------------------------------------------------------------------
#-- Python
find_package (Python COMPONENTS Interpreter Development)
if(Python_FOUND)
  message(STATUS "Found Python ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}.${Python_VERSION_PATCH}")
  if(${Python_VERSION_MAJOR} LESS 3)
    message(FATAL_ERROR "Found Python ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}; Python 3+ required!")
  endif()

  set(Python_CONFIG "${Python_EXECUTABLE}-config")

  set(SST_PYTHON_LIB "-lpython${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
  set(SST_LIB "${SST_LIB} ${SST_PYTHON_LIB}")

  execute_process(
    COMMAND bash -c "${Python_CONFIG} --includes"
    OUTPUT_VARIABLE Python_CPPFLAGS
  )
  string(REGEX REPLACE "\n$" "" Python_CPPFLAGS "${Python_CPPFLAGS}")

  execute_process(
    COMMAND bash -c "${Python_CONFIG} --ldflags"
    OUTPUT_VARIABLE Python_LDFLAGS
  )
  string(REGEX REPLACE "\n$" "" Python_LDFLAGS "${Python_LDFLAGS}")

  execute_process(
    COMMAND bash -c "${Python_CONFIG} --libs"
    OUTPUT_VARIABLE Python_LIBS
  )
  string(REGEX REPLACE "\n$" "" Python_LIBS "${Python_LIBS}")
  set(SST_LIB "${SST_LIB} ${Python_LIBS}")

  message(STATUS "Python Intpreter found at ${Python_EXECUTABLE}")
  message(STATUS "Python Config found at ${Python_CONFIG}")
  message(STATUS "Python Libraries found at ${Python_LIBRARY_DIRS}")
  message(STATUS "Python Headers found at ${Python_INCLUDE_DIRS}")
  message(STATUS "Python link options are ${Python_LINK_OPTIONS}")
  message(STATUS "Python CPPFLAGS are: ${Python_CPPFLAGS}")
  message(STATUS "Python LDFLAGS are: ${Python_LDFLAGS}")
  message(STATUS "Python LIBS are: ${Python_LIBS}")
else()
  message(FATAL_ERROR "Python 3.5+ is required for SST-CORE")
endif()

#-- LibZ
if( SST_ENABLE_LIBZ )
  find_package(ZLIB REQUIRED)
  if( ZLIB_FOUND )
    message(STATUS "LibZ Headers found at ${ZLIB_INCLUDE_DIRS}")
    message(STATUS "LibZ Libraries found at ${ZLIB_LIBRARIES}")
    set(SST_LINK_LIBS "${SST_LINK_LIBS} ${ZLIB_LIBRARIES}")
    set(SST_ZLIB_MACRO "#define HAVE_LIBZ 1")
    set(SST_LIB "${SST_LIB} ${ZLIB_LIBRARIES}")
  else()
    message(FATAL_ERROR "LibZ not found")
  endif()
else()
  message(STATUS "LibZ support disabled")
  set(ZLIB_INCLUDE_DIRS "")
  set(ZLIB_LIBRARIES "")
  set(SST_ZLIB_MACRO "/* #undef HAVE_LIBZ */")
endif()

#-- HDF5
if( SST_ENABLE_HDF5 )
  find_package(HDF5 REQUIRED)
  if( HDF5_FOUND )
    message(STATUS "HDF5 Headers found at ${HDF5_INCLUDE_DIRS}")
    message(STATUS "HDF5 Libraries found at ${HDF5_LIBRARIES}")
    message(STATUS "HDF5 Compiler definitions set to ${HDF5_DEFINITIONS}")
    set(SST_LINK_LIBS "${SST_LINK_LIBS} ${HDF5_LIBRARIES}")
    set(SST_HDF5_MACRO "#define HAVE_HDF5 1")
    set(SST_LIB "${SST_LIB} ${HDF5_LIBRARIES}")
  else()
    message(FATAL_ERROR "HDF5 not found")
  endif()
else()
  set(SST_HDF5_MACRO "/* #undef HAVE_HDF5 */")
  set(HDF5_INCLUDE_DIRS "")
  set(HDF5_LINK_LIBS "")
endif()

#-- MPI
if( SST_ENABLE_MPI )
  find_package(MPI REQUIRED)
  if( MPI_FOUND )
    message(STATUS "MPI C Compiler found at ${MPI_C_COMPILER}")
    message(STATUS "MPI CXX Compiler found at ${MPI_CXX_COMPILER}")
    message(STATUS "MPI Headers found at ${MPI_CXX_INCLUDE_DIRS}")
    message(STATUS "MPI Libraries found at ${MPI_CXX_LINK_FLAGS}")
    message(STATUS "MPI Link libraries set to ${MPI_CXX_LIBRARIES}")
    set(CMAKE_C_COMPILER "${MPI_C_COMPILER}")
    set(CMAKE_CXX_COMPILER "${MPI_CXX_COMPILER}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSST_CONFIG_HAVE_MPI=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSST_CONFIG_HAVE_MPI=1")
    set(HAVE_MPI "1")
    set(SST_MPI_MACRO "#define SST_CONFIG_HAVE_MPI ${HAVE_MPI}")
  else()
    message(FATAL_ERROR "MPI not found")
  endif()
else()
  set(MPI_C_COMPILER "${CMAKE_C_COMPILER}")
  set(MPI_CXX_COMPILER "${CMAKE_CXX_COMPILER}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(HAVE_MPI "0")
  set(SST_MPI_MACRO "/* #undef SST_CONFIG_HAVE_MPI */")
endif()

message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

#-- check for OSX
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(SST_COMPILE_OSX ON)
else()
  set(SST_COMPILE_OSX OFF)
endif()

#-- Doxygen
if(SST_BUILD_DOCUMENTATION)
find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the doxygen-ized API documentation (requires Doxygen)" ${DOXYGEN_FOUND})
endif(SST_BUILD_DOCUMENTATION)

#------------------------------------------------------------------------
#-- SST-CORE LINKER OPTIONS
#------------------------------------------------------------------------
set(SST_SIM_LDFLAGS "${Python_LDFLAGS} ${SST_PYTHON_LIB} -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions -export-dynamic ${SST_LTDL_LIBS} ${SST_LINK_LIBS}")
set(SST_INFO_LDFLAGS "${Python_LDFLAGS} ${SST_PYTHON_LIB} -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions -export-dynamic ${SST_LTDL_LIBS} ${SST_LINK_LIBS}")

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

#------------------------------------------------------------------------
#-- SST-CORE CONFIG FILE
#------------------------------------------------------------------------
message(STATUS "SST: PREPROCESSING ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/core/sst_config.h")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/core/sst_config.h
"/* src/sst/core/sst_config.h.  Generated from sst_config.h.in by configure.  */
/* src/sst/core/sst_config.h.in.  Generated from configure.ac by autoheader.  */

${SST_HAVE_ARGZ_ADD_MACRO}

${SST_HAVE_ERROR_T_MACRO}

${SST_HAVE_WORKING_ARGZ_MACRO}

${SST_HAVE_ARGZ_ADD_MACRO}

${SST_HAVE_ERROR_T_MACRO}

${SST_HAVE_WORKING_ARGZ_MACRO}

${SST_HAVE_ARGZ_APPEND_MACRO}

${SST_HAVE_ARGZ_COUNT_MACRO}

${SST_HAVE_ARGZ_CREATE_SEP_MACRO}

${SST_HAVE_ARGZ_H_MACRO}

${SST_HAVE_ARGZ_INSERT_MACRO}

${SST_HAVE_ARGZ_NEXT_MACRO}

${SST_HAVE_ARGZ_STRINGIFY_MACRO}

${SST_HAVE_CLOSEDIR_MACRO}

${SST_HAVE_C_ASM_H_MACRO}

${SST_HAVE_DECL_CYGWIN_CONV_PATH_MACRO}

${SST_HAVE_DIRENT_H_MACRO}

${SST_HAVE_DLD_MACRO}

${SST_HAVE_DLD_H_MACRO}

${SST_HAVE_DLERROR_MACRO}

${SST_HAVE_LIBDL_MACRO}

${SST_HAVE_DLERROR_MACRO}

${SST_HAVE_LIBDL_MACRO}

${SST_HAVE_DLFCN_H_MACRO}

${SST_HAVE_DL_H_MACRO}

${SST_HAVE_DYLD_MACRO}

${SST_HAVE_INTRINSICS_H_MACRO}

${SST_HAVE_INTTYPES_H_MACRO}

${SST_HAVE_LIBLTDL_MACRO}

${SST_HAVE_LTDL_MACRO}

${SST_HAVE_LIBLTDL_MACRO}

${SST_HAVE_LTDL_MACRO}

${SST_HAVE_MACH_MACH_TIME_H_MACRO}

${SST_HAVE_MACH_O_DYLD_H_MACRO}

${SST_HAVE_MEMORY_H_MACRO}

${SST_HAVE_OPENDIR_MACRO}

${SST_HAVE_PYTHON_H_MACRO}

${SST_HAVE_READDIR_MACRO}

${SST_HAVE_STDCXX_1Y_MACRO}

${SST_HAVE_STDINT_H_MACRO}

${SST_HAVE_SYS_STAT_H_MACRO}

${SST_HAVE_SYS_TIME_H_MACRO}

${SST_HAVE_SYS_TYPES_H_MACRO}

${SST_UNISTD_H_MACRO}

#define PACKAGE_BUGREPORT \"sst@sandia.gov\"

#define PACKAGE_NAME \"SSTCore\"

#define PACKAGE_STRING \"SSTCore ${VERSION}\"

#define PACKAGE_TARNAME \"sstcore\"

#define PACKAGE_VERSION \"${VERSION}\"

#define SSTCORE_GIT_BRANCH \"N/A\"

#define SSTCORE_GIT_COMMITCOUNT \"0\"

#define SSTCORE_GIT_HEADSHA \"${VERSION}\"

#define SST_CC \"${CMAKE_C_COMPILER}\"

#define SST_CFLAGS \"${CMAKE_C_FLAGS}\"

${SST_MPI_MACRO}

#define SST_CONFIG_HAVE_PYTHON 1

#define SST_CONFIG_HAVE_PYTHON3 1

#define SST_CPP \"${CMAKE_CXX_COMPILER} -E\"

#define SST_CPPFLAGS \"${SST_INCLUDE_DIRS}\"

#define SST_CXX \"${CMAKE_CXX_COMPILER}\"

#define SST_CXXFLAGS \"${CMAKE_CXX_FLAGS}\"

#define SST_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\"

#define SST_LD \"${SST_C_COMPILER}\"

#define SST_LDFLAGS \"\"

#define SST_MPICC \"${MPI_C_COMPILER}\"

#define SST_MPICXX \"${MPI_CXX_COMPILER}\"

#define SST_PYTHON_CPPFLAGS \"${Python_CPPFLAGS}\"

#define SST_PYTHON_LDFLAGS \"${Python_LDFLAGS}\"

${SST_HDF5_MACRO}

${SST_ZLIB_MACRO}
"
)

#------------------------------------------------------------------------
#-- SSTSIMULATOR.CONF FILE
#------------------------------------------------------------------------
message(STATUS "SST: PREPROCESSING ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/sstsimulator.conf")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/sstsimulator.conf
"# ===================================================================
# SST Configuration Database
#
# This file is automatically generated during installation. Any
# changes made manually may be overridden. Please see the SST
# documentation for details on how the configurations should be
# edited to prevent errors during execution
#
# ===================================================================
[SSTCore]
MPICXX=${MPI_CXX_COMPILER}
MPICC=${MPI_C_COMPILER}
MPI_CPPFLAGS=
CXX=${CMAKE_CXX_COMPILER}
CXXFLAGS=${CMAKE_CXX_FLAGS}
CC=${CMAKE_C_FLAGS}
CFLAGS=${CMAKE_C_FLAGS}
LD=${CMAKE_CXX_COMPILER}
LDFLAGS=${CMAKE_LINKER}
LIBS=-ldl
CPP=${CMAKE_C_COMPILER} -E
CXXCPP=${CMAKE_CXX_COMPILER} -E
AR=ar
NM=nm -B
PYTHON=${Python_EXECUTABLE}
PYTHON_CONFIG=${Python_CONFIG}
PYTHON_CPPFLAGS=${Python_CPPFLAGS}
PYTHON_LDFLAGS=${Python_LDFLAGS}
PYTHON_VERSION3=yes
ZOLTAN_CPPFLAGS=
ZOLTAN_LDFLAGS=
LIBZ_CPPFLAGS=${ZLIB_INCLUDE_DIRS}
LIBZ_LDFLAGS=${ZLIB_LIBRARIES}
LIBZ_LIBS=-lz
ELEMENT_CXXFLAGS= ${CMAKE_ELEMENT_CXXFLAGS}
ELEMENT_LDFLAGS= ${CMAKE_ELEMENT_LDFLAGS}
pkgconfig=${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/SST-${VERSION}.pc
sourcedir=${CMAKE_CURRENT_SOURCE_DIR}/src/sst/core
builddir=${CMAKE_CURRENT_BINARY_DIR}
testsdir=${CMAKE_CURRENT_SOURCE_DIR}/src/sst/../../tests
prefix=${CMAKE_INSTALL_PREFIX}
exec_prefix=${CMAKE_INSTALL_PREFIX}
bindir=${CMAKE_INSTALL_PREFIX}/bin
sbindir=${CMAKE_INSTALL_PREFIX}/sbin
libexecdir=${CMAKE_INSTALL_PREFIX}/libexec
includedir=${CMAKE_INSTALL_PREFIX}/include
libdir=${CMAKE_INSTALL_PREFIX}/lib
CORE_TEST_ELEMENT_LIBDIR=${CMAKE_INSTALL_PREFIX}/lib/sstcore
PREVIEW_BUILD=
")


#------------------------------------------------------------------------
#-- SST PKGCONFIG SCRIPT
#------------------------------------------------------------------------
message(STATUS "SST: PREPROCESSING ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/SST-${VERSION}.pc")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/SST-${VERSION}.pc
"prefix=${CMAKE_INSTALL_PREFIX}
exec_prefix=$\{prefix\}
libdir=$\{exec_prefix\}/lib
includedir=$\{prefix\}/include

Name: SSTCore
Description: Structural Simulation Toolkit (SST) Parallel Discrete Event Core
Version: ${VERSION}
URL: http://www.sst-simulator.org/
Libs: -Wl,-undefined -Wl,dynamic_lookup
Cflags: -DHAVE_CONFIG_H -I$\{includedir\} -I$\{includedir\}/sst
"
)

#------------------------------------------------------------------------
#-- SST-CORE BUILD INFO FILE
#------------------------------------------------------------------------
message(STATUS "SST: PREPROCESSING ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/core/build_info.h")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/core/build_info.h
"
/* -*- C -*-
 *
 * DO NOT EDIT BUILD_INFO.H!
 * It is automatically generated from build_info.h.in
 *
 * Parameters which can change from run to run based on configure
 * arguments, but which do not require a full rebuild (such as
 * installation directory) are set in this file.
 */

#ifndef SST_BUILD_INFO_H
#define SST_BUILD_INFO_H

#endif
"
)

#------------------------------------------------------------------------
#-- SST-CORE BUILD libcoreTestElement.la
#------------------------------------------------------------------------
message(STATUS "SST: PREPROCESSING ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/core/testElements/libcoreTestElement.la")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/sst/core/testElements/libcoreTestElement.la
"# libcoreTestElement.la - a libtool library file
# Generated by libtool (GNU libtool) 2.4.2
#
# Please DO NOT delete this file!
# It is necessary for linking the library.

# The name that we can dlopen(3).
dlname='libcoreTestElement.so'

# Names of this library.
library_names='libcoreTestElement.so libcoreTestElement.so libcoreTestElement.so'

# The name of the static archive.
old_library=''

# Linker flags that can not go in dependency_libs.
inherited_linker_flags=''

# Libraries that this one depends upon.
dependency_libs=' -ldl'

# Names of additional weak libraries provided by this library
weak_library_names=''

# Version information for libcoreTestElement.
current=0
age=0
revision=0

# Is this an already installed library?
installed=yes

# Should we warn about portability when linking against -modules?
shouldnotlink=yes

# Files to dlopen/dlpreopen
dlopen=''
dlpreopen=''

# Directory that this library needs to be installed in:
libdir='${CMAKE_INSTALL_PREFIX}/lib/sstcore'"
)

#------------------------------------------------------------------------
#-- TESTING
#------------------------------------------------------------------------
enable_testing()

#------------------------------------------------------------------------
#-- DOCUMENTATION
#------------------------------------------------------------------------
if(SST_BUILD_DOCUMENTATION AND BUILD_DOCUMENTATION)
  find_package(Git)
  if( NOT Git_FOUND )
    message(FATAL_ERROR "Git is required to tag the source documentation.")
  endif()
  if( NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is required to build the documentation.")
  endif()

  #-- set the preprocessor defs
  execute_process(COMMAND
    "${GIT_EXECUTABLE}" describe --match=NeVeRmAtCh --always --abbrev=40 --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE SST_SVN_REVISION
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(abs_builddir "${CMAKE_CURRENT_SOURCE_DIR}/doc")

  #-- preprocess the doxyfile
  message(STATUS "SST: PREPROCESSING ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in")
  configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.conf"
  )

  set(doxyfile ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.conf)

  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/html DESTINATION share/doc)
endif(SST_BUILD_DOCUMENTATION AND BUILD_DOCUMENTATION)


#------------------------------------------------------------------------
#-- PACKAGING
#------------------------------------------------------------------------
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
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
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
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
  include(CPack)
endif()

#------------------------------------------------------------------------
#-- SUBDIRECTORIES
#------------------------------------------------------------------------
add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(share)

# EOF
