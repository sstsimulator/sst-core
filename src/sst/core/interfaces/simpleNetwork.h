// -*- mode: c++ -*-
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

#ifndef SST_CORE_INTERFACES_SIMPLENETWORK_H
#define SST_CORE_INTERFACES_SIMPLENETWORK_H

#include "sst/core/params.h"
#include "sst/core/serialization/serializable.h"
#include "sst/core/sst_types.h"
#include "sst/core/ssthandler.h"
#include "sst/core/subcomponent.h"
#include "sst/core/warnmacros.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace SST {
class Component;
class Event;
class Link;
} // namespace SST

namespace SST::Interfaces {

/**
 * Generic network interface
 */
class SimpleNetwork : public SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Interfaces::SimpleNetwork, int)

    /** All Addresses can be 64-bit */
    using nid_t = int64_t;
#define PRI_NID PRIi64

    static const nid_t INIT_BROADCAST_ADDR;

    /** Stable, service-neutral identifiers used by optional network services. */
    using NetworkServiceID        = uint16_t;
    using NetworkServiceDataToken = uint16_t;
    using NetworkServiceVersion   = uint16_t;

    static constexpr NetworkServiceID NETWORK_SERVICE_NONE       = 0;
    static constexpr NetworkServiceID NETWORK_SERVICE_SST_MIN    = 1;
    static constexpr NetworkServiceID NETWORK_SERVICE_SST_MAX    = 0x7fff;
    static constexpr NetworkServiceID NETWORK_SERVICE_PLUGIN_MIN = 0x8000;
    static constexpr NetworkServiceID NETWORK_SERVICE_PLUGIN_MAX = 0xffff;

    /** Maximum endpoint-logical VNs accepted in a serialized capability. */
    static constexpr size_t NETWORK_SERVICE_MAX_VNS = 64 * 1024;

    /** Service-neutral transport features reported by a SimpleNetwork instance. */
    enum NetworkServiceFeature : uint64_t {
        SERVICE_FEATURE_NONE                                 = 0,
        SERVICE_FEATURE_SIDECAR_PRESERVATION                 = 1ull << 0,
        SERVICE_FEATURE_TRANSACTIONAL_TIMED_SEND             = 1ull << 1,
        SERVICE_FEATURE_SERIALIZATION                        = 1ull << 2,
        SERVICE_FEATURE_INTERMEDIATE_TERMINATION_SAFE        = 1ull << 3,
        SERVICE_FEATURE_FRESH_BASE_REQUEST_TAG_FIRST_RECEIVE = 1ull << 4
    };

    using NetworkServiceFeatureMask = uint64_t;

    /** Service-neutral network-service capability record. */
    struct NetworkServiceCapability
    {
        NetworkServiceID          service_id         = NETWORK_SERVICE_NONE;
        NetworkServiceVersion     min_schema_version = 0;
        NetworkServiceVersion     max_schema_version = 0;
        NetworkServiceFeatureMask features           = SERVICE_FEATURE_NONE;

        NetworkServiceDataToken request_data_token         = 0;
        NetworkServiceVersion   min_request_schema_version = 0;
        NetworkServiceVersion   max_request_schema_version = 0;

        /**
         * Optional maximum atomic Request size indexed by endpoint-logical
         * VN.  A zero entry means that VN is unsupported for this service;
         * unsupported trailing VNs may be omitted.
         */
        std::vector<uint64_t> max_atomic_request_bits_by_vn;

        void serialize_order(SST::Core::Serialization::serializer& ser);

        bool isValidFor(NetworkServiceID requested_id) const;
    };

    /**
     * Polymorphic service metadata owned by a Request.
     * Types with their own serialized state must register with SST serialization
     * and must deep-clone owned state. Serialization uses the standard SST pointer
     * path; it does not enforce ownership or add exception rollback. Callers must
     * manage aliases and any prior pointer when unpacking, as with native payloads.
     * The registered stream type must derive from NetworkServiceData; envelope
     * validation checks the decoded service fields.
     * A state-free subtype may retain its parent's registered representation;
     * typed inspection checks that registered identity, not an exact RTTI type.
     */
    class NetworkServiceData : public SST::Core::Serialization::serializable
    {
    public:
        ~NetworkServiceData() override = default;

        virtual NetworkServiceID        serviceID() const     = 0;
        virtual NetworkServiceDataToken dataToken() const     = 0;
        virtual NetworkServiceVersion   schemaVersion() const = 0;
        virtual NetworkServiceData*     clone() const         = 0;

        ImplementVirtualSerializable(SST::Interfaces::SimpleNetwork::NetworkServiceData)
    };

    /**
     * Represents both network sends and receives
     */
    class Request : public SST::Core::Serialization::serializable
    {

    public:
        nid_t  dest;           /*!< Node ID of destination */
        nid_t  src;            /*!< Node ID of source */
        int    vn;             /*!< Virtual network of packet */
        size_t size_in_bits;   /*!< Size of packet in bits */
        bool   head;           /*!< True if this is the head of a stream */
        bool   tail;           /*!< True if this is the tail of a steram */
        bool   allow_adaptive; /*!< Indicates whether adaptive routing is allowed or not. */

    private:
        // Keep the 16-bit discriminator before the pointers so it occupies
        // padding already required by Request's public scalar fields.
        NetworkServiceID    service_id;   /*!< Cached service discriminator */
        Event*              payload;      /*!< Native payload of the request */
        NetworkServiceData* service_data; /*!< Optional, exclusively owned service metadata */

        static NetworkServiceData* cloneService(const NetworkServiceData* source);
        void                       validateServiceInvariant() const;

    public:
        /**
           Sets the payload field for this request
           @param payload_in Event to set as payload.
           This preserves the released API behavior: an existing pointer is
           replaced without being deleted.
         */
        inline void givePayload(Event* event) { payload = event; }

        /**
           Returns the payload for the request.  This will also set
           the payload to nullptr, so the call will only return valid
           data one time after each givePayload call.
           @return Event that was set as payload of the request.
        */
        inline Event* takePayload()
        {
            Event* ret = payload;
            payload    = nullptr;
            return ret;
        }

        /**
           Returns the payload for the request for inspection.  This
           call does not set the payload to nullptr, so deleting the
           request will also delete the payload.  If the request is
           going to be deleted, use takePayload instead.
           @return Event that was set as payload of the request.
        */
        inline Event*       inspectPayload() { return payload; }
        inline const Event* inspectPayload() const { return payload; }

        /** Deletes and clears the native payload, if present. */
        inline void clearPayload()
        {
            delete payload;
            payload = nullptr;
        }

        /** Returns the cached service discriminator. */
        inline NetworkServiceID getServiceID() const { return service_id; }

        /** Returns true when a complete sidecar-backed service is installed. */
        inline bool hasService() const { return service_data != nullptr; }

        /**
         * Transfers exclusive ownership of service data into an empty slot.
         * Ownership remains with the caller if validation throws.
         */
        template <class T>
        inline void giveServiceData(T* data)
        {
            static_assert(std::is_base_of_v<NetworkServiceData, T>, "T must derive from NetworkServiceData");
            static_assert(!std::is_abstract_v<T>, "T must be a concrete NetworkServiceData type");
            if ( data == nullptr ) {
                throw std::invalid_argument("SimpleNetwork::Request service data cannot be null");
            }
            if ( service_data != nullptr || service_id != NETWORK_SERVICE_NONE ) {
                throw std::logic_error("SimpleNetwork::Request service slot is already occupied");
            }
            const NetworkServiceID id = data->serviceID();
            if ( id == NETWORK_SERVICE_NONE ) {
                throw std::invalid_argument("SimpleNetwork::Request service data uses the reserved None service ID");
            }
            if ( data->dataToken() == 0 ) {
                throw std::invalid_argument("SimpleNetwork::Request service data uses the reserved zero token");
            }
            if ( data->serviceID() != T::SERVICE_ID || data->dataToken() != T::DATA_TOKEN ||
                 data->schemaVersion() < T::MIN_SCHEMA_VERSION || data->schemaVersion() > T::MAX_SCHEMA_VERSION ) {
                throw std::invalid_argument("SimpleNetwork::Request service data contradicts its concrete contract");
            }
            if ( data->cls_id() != SST::Core::Serialization::serializable_builder_impl<T>::static_cls_id() ) {
                throw std::invalid_argument("SimpleNetwork::Request service data concrete type is not registered");
            }

            service_id   = id;
            service_data = data;
        }

        /** Non-owning inspection of generic service data. */
        inline const NetworkServiceData* inspectServiceData() const { return service_data; }

        /** Checks fixed identity fields without casting service-owned data. */
        inline bool serviceDataMatches(NetworkServiceID expected_id, NetworkServiceDataToken expected_token,
            NetworkServiceVersion minimum_version, NetworkServiceVersion maximum_version) const
        {
            const NetworkServiceData* data = inspectServiceData();
            return data != nullptr && getServiceID() == expected_id && data->serviceID() == expected_id &&
                   data->dataToken() == expected_token && data->schemaVersion() >= minimum_version &&
                   data->schemaVersion() <= maximum_version;
        }

        /**
         * Checked, non-owning typed inspection.  The cast is performed only
         * after the fixed generic identity fields declared by T have matched.
         * Data tokens are unique within one service ID.
         */
        template <class T>
        inline const T* inspectServiceDataAs() const
        {
            static_assert(std::is_base_of_v<NetworkServiceData, T>, "T must derive from NetworkServiceData");
            if ( service_id != T::SERVICE_ID || service_data == nullptr ) return nullptr;
            if ( service_data->cls_id() != SST::Core::Serialization::serializable_builder_impl<T>::static_cls_id() ) {
                return nullptr;
            }
            if ( !serviceDataMatches(T::SERVICE_ID, T::DATA_TOKEN, T::MIN_SCHEMA_VERSION, T::MAX_SCHEMA_VERSION) ) {
                return nullptr;
            }
            return static_cast<const T*>(service_data);
        }

        /** Transfers ownership out and clears the complete service envelope. */
        inline NetworkServiceData* takeServiceData()
        {
            NetworkServiceData* ret = service_data;
            service_data            = nullptr;
            service_id              = NETWORK_SERVICE_NONE;
            return ret;
        }

        /** Deletes and clears the complete service envelope. */
        inline void clearService()
        {
            delete service_data;
            service_data = nullptr;
            service_id   = NETWORK_SERVICE_NONE;
        }

        /**
         * Trace types
         */
        enum TraceType {
            NONE,  /*!< No tracing enabled */
            ROUTE, /*!< Trace route information only */
            FULL   /*!< Trace all movements of packets through network */
        };

        /** Constructor */
        Request() :
            dest(0),
            src(0),
            vn(0),
            size_in_bits(0),
            head(false),
            tail(false),
            allow_adaptive(true),
            service_id(NETWORK_SERVICE_NONE),
            payload(nullptr),
            service_data(nullptr),
            trace(NONE),
            traceID(0)
        {}

        Request(nid_t dest, nid_t src, size_t size_in_bits, bool head, bool tail, Event* payload = nullptr) :
            dest(dest),
            src(src),
            vn(0),
            size_in_bits(size_in_bits),
            head(head),
            tail(tail),
            allow_adaptive(true),
            service_id(NETWORK_SERVICE_NONE),
            payload(payload),
            service_data(nullptr),
            trace(NONE),
            traceID(0)
        {}

        /** Copies scalars, aliases the native payload (released contract), and deep-clones the sidecar. */
        Request(const Request& other);

        Request(Request&& other) noexcept :
            Request()
        {
            swap(other);
        }

        Request& operator=(const Request& other);

        Request& operator=(Request&& other) noexcept
        {
            if ( this != &other ) {
                Request moved(std::move(other));
                swap(moved);
                moved.takePayload(); // Preserve the released native-payload overwrite contract.
            }
            return *this;
        }

        ~Request() override
        {
            clearPayload();
            clearService();
        }

        void swap(Request& other) noexcept
        {
            using std::swap;
            swap(dest, other.dest);
            swap(src, other.src);
            swap(vn, other.vn);
            swap(size_in_bits, other.size_in_bits);
            swap(head, other.head);
            swap(tail, other.tail);
            swap(allow_adaptive, other.allow_adaptive);
            swap(service_id, other.service_id);
            swap(payload, other.payload);
            swap(service_data, other.service_data);
            swap(trace, other.trace);
            swap(traceID, other.traceID);
        }

        /** Deep-clones the native payload and the sidecar. */
        virtual Request* clone();

        void      setTraceID(int id) { traceID = id; }
        void      setTraceType(TraceType type) { trace = type; }
        int       getTraceID() const { return traceID; }
        TraceType getTraceType() const { return trace; }

        void serialize_order(SST::Core::Serialization::serializer& ser) override;

    protected:
        TraceType trace;
        int       traceID;

    private:
        ImplementSerializable(SST::Interfaces::SimpleNetwork::Request)
    };
    /**
       Class used to inspect network requests going through the network.
     */
    class NetworkInspector : public SubComponent
    {

    public:
        SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Interfaces::SimpleNetwork::NetworkInspector,std::string)

        explicit NetworkInspector(ComponentId_t id) :
            SubComponent(id)
        {}

        explicit NetworkInspector() :
            SubComponent()
        {}

        virtual ~NetworkInspector() {}

        virtual void inspectNetworkData(Request* req) = 0;
    };

    /**
       Base handler for event delivery.
     */
    using HandlerBase = SSTHandlerBase<bool, int>;

    /**
       Used to create checkpointable handlers to notify the endpoint
       when the SimpleNetwork sends or recieves a packet..  The
       callback function is expected to be in the form of:

         bool func(int vn)

       In which case, the class is created with:

         new SimpleNetwork::Handler<classname, &classname::function_name>(this)

       Or, to add static data, the callback function is:

         bool func(int vn, dataT data)

       and the class is created with:

         new SimpleNetwork::Handler<classname, &classname::function_name, dataT>(this, data)

       In both cases, the boolean that's returned indicates whether
       the handler should be kept in the list or not.  On return
       of true, the handler will be kept.  On return of false, the
       handler will be removed from the clock list.
    */
    template <typename classT, auto funcT, typename dataT = void>
    using Handler = SSTHandler<bool, int, classT, dataT, funcT>;

    /**
       Used to create checkpointable handlers to notify the endpoint
       when the SimpleNetwork sends or recieves a packet..  The
       callback function is expected to be in the form of:

         bool func(int vn)

       In which case, the class is created with:

         new SimpleNetwork::Handler<classname, &classname::function_name>(this)

       Or, to add static data, the callback function is:

         bool func(int vn, dataT data)

       and the class is created with:

         new SimpleNetwork::Handler<classname, &classname::function_name, dataT>(this, data)

       In both cases, the boolean that's returned indicates whether
       the handler should be kept in the list or not.  On return
       of true, the handler will be kept.  On return of false, the
       handler will be removed from the clock list.
    */
    template <typename classT, auto funcT, typename dataT = void>
    using Handler2 [[deprecated(
        "The name Handler2 has been deprecated and will be removed in SST 17. Please rename Handler2 to Handler.")]]
    = SSTHandler<bool, int, classT, dataT, funcT>;


public:
    /** Constructor, designed to be used via 'loadUserSubComponent or loadAnonymousSubComponent'. */
    explicit SimpleNetwork(SST::ComponentId_t id) :
        SubComponent(id)
    {}

    SimpleNetwork() :
        SubComponent()
    {} // For serialization

    /**
     * Queries service-neutral capability data for one service ID.  A false
     * return leaves @p out unchanged.  The source-compatible default reports
     * no support.  A true return supplies a record for which
     * out.isValidFor(id) is true.  Capabilities are authoritative only after
     * this SimpleNetwork instance is initialized.
     */
    virtual bool queryServiceCapability(NetworkServiceID UNUSED(id), NetworkServiceCapability& UNUSED(out)) const
    {
        return false;
    }

    /**
     * Sends a network request during untimed phases (init() and
     * complete()).
     * Ownership transfers unconditionally when this method is called.  The
     * caller must not inspect or delete @p req afterward.
     * @see SST::Link::sendUntimedData()
     */
    virtual void sendUntimedData(Request* req) = 0;

    /**
     * Receive any data during untimed phases (init() and complete()).
     * @see SST::Link::recvUntimedData()
     */
    virtual Request* recvUntimedData() = 0;

    // /**
    //  * Returns a handle to the underlying SST::Link
    //  */
    // virtual Link* getLink() const = 0;

    /**
     * Send a Request to the network.
     *
     * A true return transfers ownership to the implementation.  An instance
     * advertising SERVICE_FEATURE_TRANSACTIONAL_TIMED_SEND additionally
     * guarantees that a false return leaves ownership with the caller and
     * leaves the Request, its owned payloads, and all observable protocol
     * state unchanged.  Generic services require that advertised feature;
     * legacy implementations inherit the default unsupported capability
     * until they pass the conformance contract.
     */
    virtual bool send(Request* req, int vn) = 0;

    /**
     * Receive a Request from the network.
     *
     * Use this method for polling-based applications.
     * Register a handler for push-based notification of responses.
     *
     * @param vn Virtual network to receive on
     * @return nullptr if nothing is available.
     * @return Pointer to a Request response (that should be deleted)
     */
    virtual Request* recv(int vn) = 0;

    virtual void setup() override {}
    virtual void init(unsigned int UNUSED(phase)) override {}
    virtual void complete(unsigned int UNUSED(phase)) override {}
    virtual void finish() override {}

    /**
     * Checks if there is sufficient space to send on the specified
     * virtual network
     * @param vn Virtual network to check
     * @param num_bits Minimum size in bits required to have space
     * to send
     * @return true if there is space in the output, false otherwise
     */
    virtual bool spaceToSend(int vn, int num_bits) = 0;

    /**
     * Checks if there is a waiting network request request pending in
     * the specified virtual network.
     * @param vn Virtual network to check
     * @return true if a network request is pending in the specified
     * virtual network, false otherwise
     */
    virtual bool requestToReceive(int vn) = 0;

    /**
     * Registers a functor which will fire when a new request is
     * received from the network.  Note, the actual request that
     * was received is not passed into the functor, it is only a
     * notification that something is available.
     * @param functor Functor to call when request is received
     */
    virtual void setNotifyOnReceive(HandlerBase* functor) = 0;
    /**
     * Registers a functor which will fire when a request is
     * sent to the network.  Note, this only tells you when data
     * is sent, it does not guarantee any specified amount of
     * available space.
     * @param functor Functor to call when request is sent
     */
    virtual void setNotifyOnSend(HandlerBase* functor)    = 0;

    /**
     * Check to see if network is initialized.  If network is not
     * initialized, then no other functions other than init() can
     * can be called on the interface.
     * @return true if network is initialized, false otherwise
     */
    virtual bool isNetworkInitialized() const = 0;

    /**
     * Returns the endpoint ID.  Cannot be called until after the
     * network is initialized.
     * @return Endpoint ID
     */
    virtual nid_t getEndpointID() const = 0;

    /**
     * Returns the final BW of the link managed by the simpleNetwork
     * instance.  Cannot be called until after the network is
     * initialized.
     * @return Link bandwidth of associated link
     */
    virtual const UnitAlgebra& getLinkBW() const = 0;

    void serialize_order(SST::Core::Serialization::serializer& ser) override { SubComponent::serialize_order(ser); }
    ImplementVirtualSerializable(SST::Interfaces::SimpleNetwork)
};

} // namespace SST::Interfaces

#endif // SST_CORE_INTERFACES_SIMPLENETWORK_H
