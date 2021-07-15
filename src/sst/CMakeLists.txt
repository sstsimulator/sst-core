# SST-CORE src/sst/ CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

add_subdirectory(core)

install(DIRECTORY "."
        DESTINATION "etc/sst"
        FILES_MATCHING
        PATTERN "sstsimulator.conf")

install(DIRECTORY "."
        DESTINATION "lib/pkgconfig"
        FILES_MATCHING
        PATTERN "*.pc")

# EOF
