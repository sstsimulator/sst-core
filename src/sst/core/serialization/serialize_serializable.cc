// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//#include <sprockit/ptr_type.h>
//#include <sprockit/spkt_string.h>
#include <sst/core/serialization/serialize_serializable.h>
#include <iostream>

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

static const long null_ptr_id = -1;

void
size_serializable(serializable* s, serializer& ser){
  long dummy = 0;
  ser.size(dummy);
  if (s) {
    s->serialize_order(ser);
  }
}

void
pack_serializable(serializable* s, serializer& ser){
    if (s) {
    // debug_printf(dbg::serialize,
    //   "object with class id %ld: %s",
    //   s->cls_id(), s->cls_name());
    long cls_id = s->cls_id();
    ser.pack(cls_id);
    s->serialize_order(ser);
  }
  else {
    // debug_printf(dbg::serialize, "null object");
    long id = null_ptr_id;
    ser.pack(id);
  }
}

void
unpack_serializable(serializable*& s, serializer& ser){
  long cls_id;
  ser.unpack(cls_id);
  if (cls_id == null_ptr_id) {
    // debug_printf(dbg::serialize, "null pointer object");
      s = NULL;
  }
  else {
    // debug_printf(dbg::serialize, "unpacking class id %ld", cls_id);
      s = SST::Core::Serialization::serializable_factory::get_serializable(cls_id);
    s->serialize_order(ser);
    // debug_printf(dbg::serialize, "unpacked object %s", s->cls_name());
  }
}

}
}
}
}
