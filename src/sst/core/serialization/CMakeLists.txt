# ~~~
# SST-CORE src/sst/core/serialization CMake
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
# ~~~
#

set(SSTSerializationHeaders
    serializable_fwd.h
    serializable.h
    serialize_array.h
    serialize_buffer_accessor.h
    serialize_deque.h
    serialize.h
    serialize_list.h
    serialize_map.h
    serialize_packer.h
    serializer_fwd.h
    serializer.h
    serialize_serializable.h
    serialize_set.h
    serialize_sizer.h
    serialize_string.h
    serialize_unpacker.h
    serialize_vector.h
    ser_ptr_type.h
    statics.h)

install(FILES ${SSTSerializationHeaders}
        DESTINATION "include/sst/core/serialization")

# EOF
