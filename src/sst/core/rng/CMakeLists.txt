# SST-CORE src/sst/core/rng CMake
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

set(SSTRNGHeaders
    constant.h
    discrete.h
    distrib.h
    expon.h
    rng.h
    gaussian.h
    marsaglia.h
    mersenne.h
    poisson.h
    sstrng.h
    uniform.h
    xorshift.h)

install(FILES ${SSTRNGHeaders} DESTINATION "include/sst/core/rng")

# EOF
