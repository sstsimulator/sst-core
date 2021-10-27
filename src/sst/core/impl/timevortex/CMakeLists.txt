# SST-CORE src/sst/core/impl/timevortex CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_library(timeVortex OBJECT timeVortexPQ.cc)

target_include_directories(timeVortex PUBLIC ${SST_TOP_SRC_DIR}/src)
target_link_libraries(timeVortex PUBLIC sst-config-headers)

# EOF
