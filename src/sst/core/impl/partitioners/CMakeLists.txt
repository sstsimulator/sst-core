add_library(partitioner OBJECT linpart.cc rrobin.cc selfpart.cc simplepart.cc
                               singlepart.cc)

target_include_directories(partitioner PUBLIC ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(partitioner PUBLIC sst-config-headers)

# Not supporting ZOLTAN with CMake if(ZOLTAN_FOUND)
# target_sources(libpartitioner zoltpart.cc) endif(ZOLTAN_FOUND)
