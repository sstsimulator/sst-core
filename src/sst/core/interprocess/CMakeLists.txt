# SST-CORE src/sst/core/interprocess CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

set(SSTInterprocessHeaders
  circularBuffer.h
  ipctunnel.h
  mmapchild_pin3.h
  mmapparent.h
  shmchild.h
  shmparent.h
  sstmutex.h
  tunneldef.h)

install(FILES ${SSTInterprocessHeaders}
        DESTINATION "include/sst/core/interprocess")

# EOF
