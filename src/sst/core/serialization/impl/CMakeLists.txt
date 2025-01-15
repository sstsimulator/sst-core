# ~~~
# SST-CORE src/sst/core/serialization CMake
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
# ~~~
#

set(SSTSerializationImplHeaders
    mapper.h
    packer.h
    ser_buffer_accessor.h
    serialize_array.h
    serialize_atomic.h
    serialize_deque.h
    serialize_list.h
    serialize_map.h
    serialize_multiset.h
    serialize_priority_queue.h
    serialize_set.h
    serialize_string.h
    serialize_vector.h
    sizer.h
    unpacker.h)

install(FILES ${SSTSerializationImplHeaders}
        DESTINATION "include/sst/core/serialization/impl")

# EOF
