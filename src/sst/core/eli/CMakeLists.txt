# SST-CORE src/sst/core/eli CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

set(SSTELIHeaders
  categoryInfo.h
  defaultInfo.h
  elementbuilder.h
  elementinfo.h
  elibase.h
  interfaceInfo.h
  paramsInfo.h
  portsInfo.h
  statsInfo.h
  subcompSlotInfo.h)

install(FILES ${SSTELIHeaders}
        DESTINATION "include/sst/core/eli")

# EOF
