# SST-CORE src/sst/core/cfgoutput CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

set(SSTCfgOutputHeaders
  dotConfigOutput.h
  jsonConfigOutput.h
  pythonConfigOutput.h
  xmlConfigOutput.h)

install(FILES ${SSTCfgOutputHeaders}
        DESTINATION "include/sst/core/cfgoutput")

# EOF
