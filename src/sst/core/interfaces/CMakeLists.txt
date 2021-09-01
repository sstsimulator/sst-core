# SST-CORE src/sst/core/interfaces CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

set(SSTInterfacesHeaders simpleMem.h simpleNetwork.h stdMem.h stringEvent.h
                         TestEvent.h)

install(FILES ${SSTInterfacesHeaders} DESTINATION "include/sst/core/interfaces")

# EOF
