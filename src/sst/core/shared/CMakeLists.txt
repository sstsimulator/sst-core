add_library(shared OBJECT sharedObject.cc)
target_include_directories(shared PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(shared PRIVATE sst-config-headers)

if(MPI_FOUND)
  target_link_libraries(shared PRIVATE MPI::MPI_C)
endif(MPI_FOUND)
