# ~~~
# SST-CORE src/sst/core/env CMake
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

set(SSTENVHeaders envconfig.h envquery.h)

install(FILES ${SSTENVHeaders} DESTINATION "include/sst/core/env")

# EOF
