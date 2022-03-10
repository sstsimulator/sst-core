# SST-CORE src/sst/core/statapi CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
#

set(SSTStatAPIHeaders
    stataccumulator.h
    statbase.h
    statengine.h
    statfieldinfo.h
    statgroup.h
    stathistogram.h
    statnull.h
    statoutputcsv.h
    statoutput.h
    statoutputhdf5.h
    statoutputjson.h
    statoutputtxt.h
    statuniquecount.h)

install(FILES ${SSTStatAPIHeaders} DESTINATION "include/sst/core/statapi")

# EOF
