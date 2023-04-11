# SST-CORE src/sst/core/eli CMake
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

set(SSTELIHeaders
    attributeInfo.h
    categoryInfo.h
    defaultInfo.h
    elementbuilder.h
    elementinfo.h
    elibase.h
    interfaceInfo.h
    paramsInfo.h
    portsInfo.h
    profilePointInfo.h
    statsInfo.h
    simpleInfo.h
    subcompSlotInfo.h)

install(FILES ${SSTELIHeaders} DESTINATION "include/sst/core/eli")

# EOF
