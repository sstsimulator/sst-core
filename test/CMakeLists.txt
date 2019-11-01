project (SSTInstallTests)

cmake_minimum_required(VERSION 3.12)

#force debug
set(CMAKE_BUILD_TYPE Debug)

set(PYTHON_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/python)
set(SST_ROOT "" CACHE PATH "The path to SST installation")
find_path(SST_INC sst/core/component.h PATHS ${SST_ROOT}/include NO_DEFAULT_PATH)
find_program(SST_EXE sst ${SST_ROOT}/bin NO_DEFAULT_PATH)

set(CMAKE_SHARED_LIBRARY_SUFFIX_CXX ".so")

add_library(ctest SHARED component.cc)
target_compile_features(ctest PUBLIC cxx_std_11)
target_include_directories(ctest PUBLIC ${SST_INC})

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_SHARED_LINKER_FLAGS "-shared -undefined dynamic_lookup")
endif()
set(lib_location $<TARGET_FILE:component>)

set (SSTInstallTests_VERSION_MAJOR 10)
set (SSTInstallTests_VERSION_MINOR 0)

enable_testing()
#find_package(GTest REQUIRED)

#function(add_sstmac_test exec source)
#add_executable(${exec} ${source})
#target_include_directories(${exec} PUBLIC ${SSTMAC_INC})
#add_test(NAME ${exec} COMMAND ${exec} -f test.ini)
#endfunction(add_sstmac_test)
#
function(add_check_test test_name python_file)
  cmake_parse_arguments(TEST
    ""
    "TIMEOUT"
    "ARGS"
   ${ARGN}
  )
  IF (NOT TEST_TIMEOUT)
    SET(TEST_TIMEOUT 10)
  ENDIF()
  add_test(NAME ${test_name} COMMAND ${SST_EXE} --lib-path=${CMAKE_CURRENT_BINARY_DIR} ${TEST_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/config/${python_file} TIMEOUT ${TEST_TIMEOUT})
endfunction(add_check_test)


add_check_test(basic basic.py TIMEOUT 5)

