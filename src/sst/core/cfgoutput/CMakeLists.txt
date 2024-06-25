# ~~~
# SST-CORE src/sst/core/cfgoutput CMake
#
# Copyright 2009-2024 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2024, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
# ~~~
#

set(SSTCfgOutputHeaders dotConfigOutput.h jsonConfigOutput.h
                        pythonConfigOutput.h xmlConfigOutput.h)

install(FILES ${SSTCfgOutputHeaders} DESTINATION "include/sst/core/cfgoutput")

# EOF
