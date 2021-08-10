add_library(
  modelpython OBJECT
  pymodel.cc pymodel_link.cc pymodel_comp.cc pymodel_unitalgebra.cc
  pymodel_stat.cc pymodel_statgroup.cc)


target_include_directories(modelpython PUBLIC ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(modelpython PRIVATE Python::Python sst-config-headers)

if(MPI_FOUND)
  target_link_libraries(modelpython PRIVATE MPI::MPI_C)
endif(MPI_FOUND)
