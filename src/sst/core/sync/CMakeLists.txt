add_library(sync OBJECT rankSyncParallelSkip.cc rankSyncSerialSkip.cc
                        syncManager.cc syncQueue.cc threadSyncSimpleSkip.cc)

target_compile_definitions(sync PRIVATE SST_BUILDING_CORE=1)
target_include_directories(sync PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(sync PRIVATE sst-config-headers)

if(MPI_FOUND)
  target_link_libraries(sync PRIVATE MPI::MPI_C)
endif(MPI_FOUND)
