add_library(libpartitioner OBJECT linpart.cc rrobin.cc selfpart.cc
                                  simplepart.cc singlepart.cc)

                                #target_include_directories(libpartitioner
                                #                           PUBLIC ${CMAKE_BINARY_DIR}/src/sst/core})

# Not supporting ZOLTAN with CMake if(ZOLTAN_FOUND)
# target_sources(libpartitioner zoltpart.cc) endif(ZOLTAN_FOUND)
