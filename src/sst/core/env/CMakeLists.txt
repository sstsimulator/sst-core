# SST-CORE src/sst/core/env CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

set(SSTENVHeaders
  envconfig.h
  envquery.h)

install(FILES ${SSTENVHeaders}
        DESTINATION "include/sst/core/env")

# EOF
