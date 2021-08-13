# SST-CORE src/sst/core/rng CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

set(SSTRNGHeaders
  constant.h
  discrete.h
  distrib.h
  expon.h
  gaussian.h
  marsaglia.h
  mersenne.h
  poisson.h
  sstrng.h
  uniform.h
  xorshift.h)

install(FILES ${SSTRNGHeaders}
        DESTINATION "include/sst/core/rng")

# EOF
