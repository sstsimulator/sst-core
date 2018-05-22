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

#ifndef SERIALIZE_STRING_H
#define SERIALIZE_STRING_H

#include <sst/core/serialization/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {

template <>
class serialize<std::string> {
 public:
 void operator()(std::string& str, serializer& ser){
   ser.string(str);
 }
};

}
}
}

#endif // SERIALIZE_STRING_H
