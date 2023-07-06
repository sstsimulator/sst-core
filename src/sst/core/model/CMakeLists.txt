# ~~~
# SST-CORE src/sst/core/model CMake
#
# Copyright 2009-2023 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2023, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
# ~~~

add_subdirectory(python)
add_subdirectory(json)

add_library(modelCore OBJECT sstmodel.cc element_python.cc)
target_include_directories(modelCore PUBLIC ${SST_TOP_SRC_DIR}/src)
target_link_libraries(modelCore PRIVATE sst-config-headers Python::Python)

install(FILES xmlToPython.py DESTINATION libexec)
install(FILES element_python.h DESTINATION "include/sst/core/model")

# EOF
