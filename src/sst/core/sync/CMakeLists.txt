add_library(libsync OBJECT rankSyncParallelSkip.cc rankSyncSerialSkip.cc
                           syncManager.cc syncQueue.cc threadSyncSimpleSkip.cc)

target_include_directories(libsync PUBLIC 
  ${CMAKE_BINARY_DIR}/src/sst/core
  ${CMAKE_SOURCE_DIR}/src
  )
