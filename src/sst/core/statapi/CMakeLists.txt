# SST-CORE src/sst/core/statapi CMake
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
