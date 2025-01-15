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

add_subdirectory(impl)

set(SSTSerializationHeaders
    objectMap.h
    objectMapDeferred.h
    serializable.h
    serializable_base.h
    serialize.h
    serialize_impl_fwd.h
    serialize_serializable.h
    serializer.h
    serializer_fwd.h
    statics.h)

install(FILES ${SSTSerializationHeaders}
        DESTINATION "include/sst/core/serialization")

# EOF
