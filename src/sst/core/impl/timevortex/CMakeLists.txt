add_library(timeVortex OBJECT timeVortexPQ.cc)

target_include_directories(timeVortex PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(timeVortex PUBLIC sst-config-headers)
