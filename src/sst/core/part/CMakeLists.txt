# SST-CORE src/sst/core/part CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

set(SSTPartHeaders
  sstpart.h)

install(FILES ${SSTPartHeaders}
        DESTINATION "include/sst/core/part")

# EOF
