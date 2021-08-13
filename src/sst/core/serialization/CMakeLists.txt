# SST-CORE src/sst/core/serialization CMake
#
# Copyright 2009-2021 National Technology and Engineering Solutions of Sandia,
# LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
# retains certain rights in this software.
#
# See LICENSE for full license details
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

install(FILES ${SSTSerialziationHeaders}
        DESTINATION "include/sst/core/serialization")

# EOF
