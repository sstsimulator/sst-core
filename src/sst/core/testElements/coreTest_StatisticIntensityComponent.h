#include "sst/core/component.h"
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>
#include <sst/core/link.h>
#include <sst/core/subcomponent.h>
#include <sst/core/statapi/stataccumulator.h>

namespace SST {
namespace CoreTestStatisticIntensityComponent {

/**
 * The code below implements the famous Hailstones problem from math
 * Components propagate events to partners. The new event is the next number
 * in the hailstone sequence
 *    N(i+1) = N/2 if i even
 *    N(i+1) = 3*N + 1 if i odd
 * The sequence evetually "converges" to 1, ending the simulation
 */

template <class T, class Fxn>
Event::HandlerBase*
newPortHandler(T* t, Fxn f){
  return new Event::Handler<T>(t, f);
}

struct HailstoneEvent :
  public Event,
  public SST::Core::Serialization::serializable_type<HailstoneEvent>
{
  ImplementSerializable(HailstoneEvent)

 public:
  HailstoneEvent(int n, int step) : n_(n), step_(step) {}

  int n() const {
    return n_;
  }

  int step() {
    return step_;
  }

  void serialize_order(Core::Serialization::serializer& ser) override {
    Event::serialize_order(ser);
    ser & n_;
    ser& step_;
  }

  HailstoneEvent(){} //for serialization

 private:
  int n_;
  int step_;
};

struct InitEvent :
  public Event,
  public SST::Core::Serialization::serializable_type<InitEvent> {

  ImplementSerializable(InitEvent)

 public:
  InitEvent(int phase) : phase_(phase){}

  void serialize_order(SST::Core::Serialization::serializer &ser) override {
    Event::serialize_order(ser);
    ser & phase_;
  }

  InitEvent(){} //for serialiation
 private:
  int phase_;
};

class coreTestStatisticIntensityPort : public SubComponent
{
 public:
  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CoreTestStatisticIntensityComponent::coreTestStatisticIntensityPort, int, const std::vector<int>&)

  virtual void handleEvent(Event* ev) = 0;

  void startHailstone(Event* ev);

  void init(unsigned int phase) override;

 protected:
  coreTestStatisticIntensityPort(ComponentId_t id, Params& UNUSED(params),
            int port, const std::vector<int>& num_init_events);

 private:
  std::vector<int> num_init_events_;

 protected:
  Link* link_;
  Link* self_link_;
  int port_;

};

class coreTestStatisticIntensityActivePort : public coreTestStatisticIntensityPort
{
 public:
  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
    coreTestStatisticIntensityActivePort,
    "coreTestElement",
    "coreTestStatisticIntensityActivePort",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "active port that propagates events",
    SST::CoreTestStatisticIntensityComponent::coreTestStatisticIntensityPort
  )

  SST_ELI_DOCUMENT_STATISTICS(
      { "num_events",     "Count number of events sent on link", "events", 1},
      { "observed_numbers",     "Track the different numbers observed", "events", 1},
      { "observations",     "Track the different numbers observed", "events", 1},
      { "traffic_intensity",    "Count the traffic on a port", "unit of traffic", 1},
  )

  SST_ELI_DOCUMENT_PORTS(
      {"outport",  "Ports which connect to other Ctest components", {} }
  )

  void setup() override;

  void handleEvent(Event *ev) override;



  coreTestStatisticIntensityActivePort(ComponentId_t id, Params& params,
             int port, const std::vector<int>& num_init_events);

 private:
  int seed_;
  int port_;

  Statistic<int>* num_events_;
  Statistic<int>* observed_numbers_;
  MultiStatistic<uint64_t, double>* traffic_intensity_;

};

class coreTestStatisticIntensityInactivePort : public coreTestStatisticIntensityPort
{
 public:
  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(coreTestStatisticIntensityInactivePort,
   "coreTestElement",
   "coreTestStatisticIntensityInactivePort",
   SST_ELI_ELEMENT_VERSION(1,0,0),
   "inactive port that does not propagate events",
   SST::CoreTestStatisticIntensityComponent::coreTestStatisticIntensityPort
  )

  void handleEvent(Event *ev) override;

  coreTestStatisticIntensityInactivePort(ComponentId_t id, Params& params,
               int port, const std::vector<int>& num_init_events);
};

class coreTestStatisticIntensityComponent : public Component {
 public:
  SST_ELI_REGISTER_COMPONENT(
      coreTestStatisticIntensityComponent,
      "coreTestElement",
      "coreTestStatisticIntensityComponent",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Ctest Component",
      COMPONENT_CATEGORY_NETWORK)

  SST_ELI_DOCUMENT_PARAMS(
      {"id", "ID of the router"},
      {"num_ports", "The number of ports"},
      {"num_init_events", "The number of events to send in each init phase"},
  )

  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
  )

 public:
  coreTestStatisticIntensityComponent(ComponentId_t cid, Params& params);

  ~coreTestStatisticIntensityComponent(){}

  void init(unsigned int phase);

  void complete(unsigned int phase);

  void setup();

  void finish();

 private:
  std::vector<coreTestStatisticIntensityPort*> ports_;

};

}
}
