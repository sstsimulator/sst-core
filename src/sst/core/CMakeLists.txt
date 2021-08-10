# SST-CORE src/sst/core CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_definitions(-DSST_BUILDING_CORE=1)

add_subdirectory(impl)
add_subdirectory(model)
add_subdirectory(shared)
add_subdirectory(sync)
add_subdirectory(testingframework)
add_subdirectory(testElements)

add_library(
  sst-core-lib OBJECT
  action.cc
  clock.cc
  baseComponent.cc
  component.cc
  componentExtension.cc
  componentInfo.cc
  config.cc
  configGraph.cc
  cfgoutput/pythonConfigOutput.cc
  cfgoutput/dotConfigOutput.cc
  cfgoutput/xmlConfigOutput.cc
  cfgoutput/jsonConfigOutput.cc
  eli/elibase.cc
  eli/elementinfo.cc
  elemLoader.cc
  event.cc
  exit.cc
  factory.cc
  heartbeat.cc
  initQueue.cc
  link.cc
  memuse.cc
  oneshot.cc
  output.cc
  params.cc
  pollingLinkQueue.cc
  simulation.cc
  subcomponent.cc
  sharedRegion.cc
  timeLord.cc
  uninitializedQueue.cc
  unitAlgebra.cc
  module.cc
  sstpart.cc
  timeVortex.cc
  serialization/serializable.cc
  serialization/serialize_serializable.cc
  serialization/serializer.cc
  serialization/statics.cc
  interfaces/TestEvent.cc
  interfaces/simpleMem.cc
  interfaces/stdMem.cc
  interfaces/simpleNetwork.cc
  interprocess/ipctunnel.cc
  interprocess/mmapparent.cc
  interprocess/shmparent.cc
  rng/marsaglia.cc
  rng/mersenne.cc
  rng/xorshift.cc
  statapi/statengine.cc
  statapi/statgroup.cc
  statapi/statoutput.cc
  statapi/statfieldinfo.cc
  statapi/statoutputconsole.cc
  statapi/statoutputtxt.cc
  statapi/statoutputcsv.cc
  statapi/statoutputjson.cc
  statapi/statbase.cc
  cputimer.cc
  iouse.cc)

target_include_directories(sst-core-lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(sst-core-lib PUBLIC SST_BUILDING_CORE=1)
#if(USE_MEMPOOL)
#  target_compile_definitions(sst-core-lib PUBLIC USE_MEMPOOL=1)
#endif(USE_MEMPOOL)

if(SST_ENABLE_EVENT_TRACKING)
  target_compile_definitions(sst-core-lib PUBLIC __SST_DEBUG_EVENT_TRACKING__=1)
endif(SST_ENABLE_EVENT_TRACKING)

if(SST_ENABLE_DEBUG_OUTPUT)
  target_compile_definitions(sst-core-lib PUBLIC __SST_DEBUG_OUTPUT__=1)
endif(SST_ENABLE_DEBUG_OUTPUT)

target_link_libraries(
  sst-core-lib
  PUBLIC nlohjson
         Python::Python
         sst-config-headers
         ${CMAKE_DL_LIBS}
         m
         )

if(MPI_FOUND)
  target_link_libraries(sst-core-lib PUBLIC MPI::MPI_C)
endif(MPI_FOUND)

if(HDF5_FOUND)
  target_link_libraries(sst-core-lib PRIVATE HDF5::HDF5)
endif(HDF5_FOUND)

if(ZLIB_FOUND)
  target_link_libraries(sst-core-lib PRIVATE ZLIB::ZLIB)
endif(ZLIB_FOUND)

add_library(sst-boot-lib OBJECT bootshared.cc)
target_link_libraries(sst-boot-lib PUBLIC sst-config-headers)
target_include_directories(sst-boot-lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_library(sst-env-lib OBJECT env/envquery.cc env/envconfig.cc)
target_link_libraries(sst-env-lib PUBLIC sst-config-headers)
target_include_directories(sst-env-lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_executable(sstsim.x main.cc)
target_link_libraries(sstsim.x 
  PRIVATE 
  sst-core-lib
  sst-env-lib
  partitioner
  timeVortex
  modelCore
  modelpython
  sync
  shared
  )
target_link_options(sstsim.x PRIVATE "LINKER:-export-dynamic")

add_executable(sstinfo.x sstinfo.cc)
target_link_libraries(sstinfo.x PRIVATE 
  sst-core-lib 
  sst-env-lib
  partitioner
  timeVortex
  modelCore
  modelpython
  sync
  shared
  tinyxml 
  )
target_link_options(sstsim.x PRIVATE "LINKER:-export-dynamic")

if(NOT APPLE)
  message(STATUS "NOT APPLE WHAT")
  target_link_options(sstinfo.x PRIVATE "LINKER:-lrt")
  target_link_options(sstsim.x PRIVATE "LINKER:-lrt")
endif(NOT APPLE)


add_executable(sst bootsst.cc)
target_link_libraries(sst PRIVATE sst-boot-lib sst-env-lib)

add_executable(sst-info bootsstinfo.cc)
target_link_libraries(sst-info PRIVATE sst-boot-lib sst-env-lib)

add_executable(sst-config sstconfigtool.cc)
target_link_libraries(sst-config PRIVATE sst-env-lib)

add_executable(sst-register sstregistertool.cc)
target_link_libraries(sst-register PRIVATE sst-env-lib)

# set_target_properties(sst PROPERTIES LINK_FLAGS ${SST_SIM_LDFLAGS})
# set_target_properties(sst-info PROPERTIES LINK_FLAGS ${SST_INFO_LDFLAGS})

install(TARGETS sst sst-info sst-config sst-register)
install(TARGETS sstsim.x sstinfo.x DESTINATION libexec)

# Don't build this it's an old thing that nots needed by cmake install(FILES
# testElements/libcoreTestElement.la DESTINATION lib/sstcore)

install(
  DIRECTORY "."
  DESTINATION "include/sst/core"
  FILES_MATCHING
  PATTERN "*.h")
