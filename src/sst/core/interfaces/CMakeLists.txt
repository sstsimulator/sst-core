# SST-CORE src/sst/core/interfaces CMake
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

set(SSTInterfacesHeaders simpleMem.h simpleNetwork.h stdMem.h stringEvent.h
                         TestEvent.h)

install(FILES ${SSTInterfacesHeaders} DESTINATION "include/sst/core/interfaces")

# EOF
