# SST-CORE src/sst/core/impl/timevortex CMake
#
# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
#

add_library(timeVortex OBJECT timeVortexPQ.cc)

target_include_directories(timeVortex PUBLIC ${SST_TOP_SRC_DIR}/src)
target_link_libraries(timeVortex PUBLIC sst-config-headers)

# EOF
