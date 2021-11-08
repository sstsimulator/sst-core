# SST-CORE src/sst/core/model CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_subdirectory(python)
add_subdirectory(json)

add_library(modelCore OBJECT sstmodel.cc element_python.cc)
target_include_directories(modelCore PUBLIC ${SST_TOP_SRC_DIR}/src)
target_link_libraries(modelCore PRIVATE sst-config-headers Python::Python)

install(FILES xmlToPython.py DESTINATION libexec)
install(FILES element_python.h DESTINATION "include/sst/core/model")

# EOF
