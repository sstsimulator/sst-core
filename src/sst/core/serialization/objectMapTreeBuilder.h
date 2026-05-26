#ifndef SST_CORE_SERIALIZATION_OBJECTMAP_TREE_BUILDERS_H
#define SST_CORE_SERIALIZATION_OBJECTMAP_TREE_BUILDERS_H

#include "sst/core/serialization/ObjTreeLeaves.h"
#include "sst/core/serialization/objectMap.h"

namespace SST::Core::Serialization {

template <typename T, typename REF, typename PTYPE>
SST::Core::Serialization::ObjTreeCont*
ObjectMapFundamentalReference<T, REF, PTYPE>::buildTreeNode(const std::string& name)
{
    using namespace SST::Core::Serialization;

    // Capture the proxy by value. The proxy holds a back-pointer
    // into the live storage (bitset/vector<bool>/atomic), so reads
    // and writes on the captured copy reflect live state.

    REF          proxy = ref;
    ObjTreeCont* node  = nullptr;

    // if order matters here - check bool first as is_integral may also match on bool
    if constexpr ( std::is_same_v<T, bool> ) {
        node =
            new BoolObj([proxy]() mutable { return static_cast<bool>(proxy); }, [proxy](bool v) mutable { proxy = v; });
    }
    else if constexpr ( std::is_integral_v<T> ) {
        node = new IntegerObj([proxy]() mutable { return static_cast<int64_t>(static_cast<T>(proxy)); },
            [proxy](int64_t v) mutable { proxy = static_cast<T>(v); });
    }
    else if constexpr ( std::is_floating_point_v<T> ) {
        node = new FloatObj([proxy]() mutable { return static_cast<long double>(static_cast<T>(proxy)); },
            [proxy](long double v) mutable { proxy = static_cast<T>(v); });
    }
    else {
        return nullptr;
    }

    node->setName(name);
    node->setType(this->getType());
    if ( this->isReadOnly() ) {
        if ( auto* b = dynamic_cast<BoolObj*>(node) ) b->makeReadOnly();
        if ( auto* i = dynamic_cast<IntegerObj*>(node) ) i->makeReadOnly();
        if ( auto* f = dynamic_cast<FloatObj*>(node) ) f->makeReadOnly();
    }
    return node;
}

} // namespace SST::Core::Serialization

#endif
