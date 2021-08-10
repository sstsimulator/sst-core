add_library(
  libmodelpython OBJECT
  pymodel.cc pymodel_link.cc pymodel_comp.cc pymodel_unitalgebra.cc
  pymodel_stat.cc pymodel_statgroup.cc)

target_include_directories(libmodelpython
                           PUBLIC ${CMAKE_BINARY_DIR}/src/sst/core)
