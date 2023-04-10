# SST-CORE src/sst/core/impl/partitioners CMake
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
#

add_library(partitioner OBJECT linpart.cc rrobin.cc selfpart.cc simplepart.cc
                               singlepart.cc)

target_include_directories(partitioner PUBLIC ${SST_TOP_SRC_DIR}/src/)
target_link_libraries(partitioner PUBLIC sst-config-headers)

# Not supporting ZOLTAN with CMake if(ZOLTAN_FOUND)
# target_sources(libpartitioner zoltpart.cc) endif(ZOLTAN_FOUND)

# EOF
