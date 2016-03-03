#ifndef SERIALIZE_UNITALGEBRA_H
#define SERIALIZE_UNITALGEBRA_H

#include <sst/core/unitAlgebra.h>
#include <sst/core/serialization/serializer.h>

namespace SST {
namespace Core {
namespace Serialization {

template <>
class serialize <UnitAlgebra> {
public:
void
operator()(UnitAlgebra& v, serializer& ser) {

    switch(ser.mode()) {
    case serializer::SIZER:
    case serializer::PACK: {
        // Get the string out of the UnitAlgebra and serialize it
        std::string str = v.toString();
        ser.string(str);
        break;
    }
    case serializer::UNPACK: {
        size_t size;
        ser.unpack(size);
        for (int i=0; i < size; ++i){
            T t;
            serialize<T>()(t, ser);
            v.push_back(t);
        }
        break;
    }
    }
}

};

}
}
}
#endif // SERIALIZE_LIST_H
