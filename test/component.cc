#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>
#include <sst/core/link.h>
#include <sst/core/subcomponent.h>
#include <sst/core/statapi/stataccumulator.h>

namespace SST {
namespace Ctest {

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

class CtestPort : public SubComponent
{
 public:
  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Ctest::CtestPort, int, const std::vector<int>&)

  virtual void handleEvent(Event* ev) = 0;

  void startHailstone(Event* ev){
    //just forward along
    link_->send(ev);
  }

  void init(unsigned int phase) override {
    if (link_ == nullptr){
      return;
    }

    if (phase > 0){
      while (auto* ev = link_->recvInitData()){
        delete ev;
      }
    }

    if (phase < num_init_events_.size()){
      int num_events = num_init_events_[phase];
      for (int e=0; e < num_events; ++e){
        auto* ev = new InitEvent(phase);
        link_->sendInitData(ev);
      }
    }
  }

 protected:
  CtestPort(ComponentId_t id, Params& UNUSED(params), int port, const std::vector<int>& num_init_events) :
    SubComponent(id),
    port_(port),
    num_init_events_(num_init_events)
  {
    std::string port_name = std::string("port") + std::to_string(port);
    link_ = configureLink(port_name, "1ps",
                          newPortHandler(this, &CtestPort::handleEvent));
    std::string self_port_name = std::string("self-port") + std::to_string(port);
    self_link_ = configureSelfLink(self_port_name, "1ps",
                                   newPortHandler(this, &CtestPort::startHailstone));
  }

 private:
  std::vector<int> num_init_events_;

 protected:
  Link* link_;
  Link* self_link_;
  int port_;

};

class ActivePort : public CtestPort
{
 public:
  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
    ActivePort,
    "ctest",
    "active",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "active port that propagates events",
    SST::Ctest::CtestPort
  )

  void setup() override {
    HailstoneEvent* ev = new HailstoneEvent(seed_, 0);
    self_link_->send(ev);
  }

  void handleEvent(Event *ev) override {
    HailstoneEvent* hev = dynamic_cast<HailstoneEvent*>(ev);
    if (hev->n() != 1){
      int newN = ((hev->n()%2)==0) ? hev->n() / 2 : 3*hev->n() + 1;
      HailstoneEvent* new_ev = new HailstoneEvent(newN, hev->step() + 1);
      link_->send(new_ev);
    }

    num_events_->addData(1);
    observed_numbers_->addData(hev->n());
    delete hev;
  }



  ActivePort(ComponentId_t id, Params& params, int port, const std::vector<int>& num_init_events) :
    CtestPort(id, params, port, num_init_events)
  {
    if (params.contains("seed")){
      seed_ = params.find<int>("seed");
    } else {
      seed_ = 10*port + 1;
    }
    num_events_ = registerStatistic<int>("num_events", std::to_string(port));
    observed_numbers_ = registerStatistic<int>("observed_numbers", std::to_string(port));
    std::cout << "Port is " << typeid(*observed_numbers_).name() << std::endl;
  }

 private:
  int seed_;

  Statistic<int>* num_events_;
  Statistic<int>* observed_numbers_;

};

class InactivePort : public CtestPort
{
 public:
  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(InactivePort,
   "ctest",
   "inactive",
   SST_ELI_ELEMENT_VERSION(1,0,0),
   "inactive port that does not propagate events",
   SST::Ctest::CtestPort
  )

  void handleEvent(Event *ev) override {
    //do nothing, just drop the event and don't propagate
    delete ev;
  }

  InactivePort(ComponentId_t id, Params& params, int port, const std::vector<int>& num_init_events) :
   CtestPort(id, params, port, num_init_events)
  {
  }
};

class CtestComponent : public Component {
 public:
  SST_ELI_REGISTER_COMPONENT(
      CtestComponent,
      "ctest",
      "ctest",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Ctest Component",
      COMPONENT_CATEGORY_NETWORK)

  SST_ELI_DOCUMENT_PARAMS(
      {"id", "ID of the router"},
      {"num_ports", "The number of ports"},
      {"num_init_events", "The number of events to send in each init phase"},
  )

  SST_ELI_DOCUMENT_STATISTICS(
      { "num_events",     "Count number of events sent on link", "events", 1},
      { "observed_numbers",     "Track the different numbers observed", "events", 1},
  )

  SST_ELI_DOCUMENT_PORTS(
      {"port%(num_ports)d",  "Ports which connect to other Ctest components", {} }
  )

  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
  )

 public:
  CtestComponent(ComponentId_t cid, Params& params) : Component(cid) {
    std::vector<int> num_init_events;
    params.find_array("num_init_events", num_init_events);

    std::vector<int> active_ports_vec;
    params.find_array("active_ports", active_ports_vec);
    //convert this to a set for easier searching
    std::set<int> active_ports(active_ports_vec.begin(), active_ports_vec.end());

    int num_ports = params.find<int>("num_ports");
    ports_.resize(num_ports);
    for (int p=0; p < num_ports; ++p){
      std::string port_type = active_ports.find(p) != active_ports.end() ? "ctest.active" : "ctest.inactive";
      std::string port_name = std::string("port") + std::to_string(p);
      ports_[p] = loadAnonymousSubComponent<CtestPort>(port_type, port_name, p,
                                                       ComponentInfo::SHARE_PORTS | ComponentInfo::SHARE_STATS,
                                                       params, p, num_init_events);
    }
  }

  ~CtestComponent(){}

  void init(unsigned int phase){
    for (auto* port : ports_){
      port->init(phase);
    }
  }

  void complete(unsigned int phase){
  }

  void setup(){
    for (auto* port : ports_){
      port->setup();
    }
  }

  void finish(){
  }

 private:
  std::vector<CtestPort*> ports_;

};

}
}
