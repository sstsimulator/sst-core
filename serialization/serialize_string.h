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
