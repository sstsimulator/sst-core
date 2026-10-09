// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//
#include "sst_config.h"

#include "sst/core/interfaces/simpleNetwork.h"

#include "sst/core/objectComms.h"

#include <memory>
#include <stdexcept>

namespace SST::Interfaces {

const SimpleNetwork::nid_t SimpleNetwork::INIT_BROADCAST_ADDR = 0xffffffffffffffffl;

void
SimpleNetwork::NetworkServiceCapability::serialize_order(SST::Core::Serialization::serializer& ser)
{
    SST_SER(service_id);
    SST_SER(min_schema_version);
    SST_SER(max_schema_version);
    SST_SER(features);
    SST_SER(request_data_token);
    SST_SER(min_request_schema_version);
    SST_SER(max_request_schema_version);

    if ( ser.mode() == SST::Core::Serialization::serializer::MAP ) {
        SST_SER(max_atomic_request_bits_by_vn);
        return;
    }

    size_t vn_count = max_atomic_request_bits_by_vn.size();
    SST_SER(vn_count);
    if ( vn_count > NETWORK_SERVICE_MAX_VNS ) {
        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            max_atomic_request_bits_by_vn.clear();
        }
        throw std::length_error("SimpleNetwork service capability VN count exceeds the maximum size");
    }
    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        max_atomic_request_bits_by_vn.resize(vn_count);
    }
    for ( auto& maximum_bits : max_atomic_request_bits_by_vn ) {
        SST_SER(maximum_bits);
    }
}

bool
SimpleNetwork::NetworkServiceCapability::isValidFor(NetworkServiceID requested_id) const
{
    if ( requested_id == NETWORK_SERVICE_NONE || service_id != requested_id ||
         min_schema_version > max_schema_version ) {
        return false;
    }
    if ( (features & SERVICE_FEATURE_SIDECAR_PRESERVATION) == 0 ) return false;
    if ( request_data_token == 0 ) {
        if ( min_request_schema_version != 0 || max_request_schema_version != 0 ) return false;
    }
    else {
        if ( min_request_schema_version > max_request_schema_version ) return false;
    }
    return max_atomic_request_bits_by_vn.size() <= NETWORK_SERVICE_MAX_VNS;
}

SimpleNetwork::NetworkServiceData*
SimpleNetwork::Request::cloneService(const NetworkServiceData* source)
{
    if ( source == nullptr ) return nullptr;

    NetworkServiceData* raw_clone = source->clone();
    if ( raw_clone == nullptr ) {
        throw std::runtime_error("SimpleNetwork::Request service data clone returned null");
    }
    if ( raw_clone == source ) {
        throw std::runtime_error("SimpleNetwork::Request service data clone returned its source");
    }

    return raw_clone;
}

void
SimpleNetwork::Request::validateServiceInvariant() const
{
    if ( (service_id == NETWORK_SERVICE_NONE) != (service_data == nullptr) ) {
        throw std::logic_error("SimpleNetwork::Request has an incomplete service envelope");
    }
    if ( service_data == nullptr ) return;
    if ( service_data->serviceID() != service_id ) {
        throw std::logic_error("SimpleNetwork::Request service ID does not match its service data");
    }
    if ( service_data->dataToken() == 0 ) {
        throw std::logic_error("SimpleNetwork::Request service data has the reserved zero token");
    }
}

SimpleNetwork::Request::Request(const Request& other) :
    Request()
{
    other.validateServiceInvariant();

    std::unique_ptr<NetworkServiceData> service_clone(cloneService(other.service_data));

    dest           = other.dest;
    src            = other.src;
    vn             = other.vn;
    size_in_bits   = other.size_in_bits;
    head           = other.head;
    tail           = other.tail;
    allow_adaptive = other.allow_adaptive;
    trace          = other.trace;
    traceID        = other.traceID;
    payload        = other.payload;
    service_id     = other.service_id;
    service_data   = service_clone.release();
}

SimpleNetwork::Request&
SimpleNetwork::Request::operator=(const Request& other)
{
    if ( this != &other ) {
        other.validateServiceInvariant();
        std::unique_ptr<NetworkServiceData> service_clone(cloneService(other.service_data));

        dest           = other.dest;
        src            = other.src;
        vn             = other.vn;
        size_in_bits   = other.size_in_bits;
        head           = other.head;
        tail           = other.tail;
        allow_adaptive = other.allow_adaptive;
        payload        = other.payload;
        clearService();
        service_id   = other.service_id;
        service_data = service_clone.release();
        trace        = other.trace;
        traceID      = other.traceID;
    }
    return *this;
}

SimpleNetwork::Request*
SimpleNetwork::Request::clone()
{
    std::unique_ptr<Request> request_clone(new Request(*this));
    request_clone->payload = nullptr;
    if ( payload != nullptr ) {
        request_clone->payload = payload->clone();
        if ( request_clone->payload == nullptr ) {
            throw std::runtime_error("SimpleNetwork::Request native payload clone returned null");
        }
    }
    return request_clone.release();
}

void
SimpleNetwork::Request::serialize_order(SST::Core::Serialization::serializer& ser)
{
    if ( ser.mode() != SST::Core::Serialization::serializer::UNPACK ) {
        validateServiceInvariant();
    }

    SST_SER(dest);
    SST_SER(src);
    SST_SER(vn);
    SST_SER(size_in_bits);
    SST_SER(head);
    SST_SER(tail);
    SST_SER(payload);
    SST_SER(trace);
    SST_SER(traceID);
    SST_SER(allow_adaptive);
    SST_SER(service_id);
    SST_SER(service_data);

    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        validateServiceInvariant();
    }
}

} // namespace SST::Interfaces
