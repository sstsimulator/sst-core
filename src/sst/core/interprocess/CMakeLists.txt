# SST-CORE src/sst/core/interprocess CMake
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
