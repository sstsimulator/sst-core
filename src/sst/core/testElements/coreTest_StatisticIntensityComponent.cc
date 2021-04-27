// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst/core/testElements/coreTest_StatisticIntensityComponent.h"

namespace SST {
namespace CoreTestStatisticIntensityComponent {

void coreTestStatisticIntensityPort::startHailstone(Event *ev){
    //just forward along
    link_->send(ev);
}

void coreTestStatisticIntensityPort::init(unsigned int phase) {
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

coreTestStatisticIntensityPort::coreTestStatisticIntensityPort(ComponentId_t id, Params &UNUSED(params), int port, const std::vector<int> &num_init_events) :
    SubComponent(id),
    port_(port),
    num_init_events_(num_init_events)
{
    link_ = configureLink("outport", "1ps",
                          newPortHandler(this, &coreTestStatisticIntensityPort::handleEvent));
    std::string self_port_name = std::string("self-port") + std::to_string(port);
    self_link_ = configureSelfLink(self_port_name, "1ps",
                                   newPortHandler(this, &coreTestStatisticIntensityPort::startHailstone));
}

void coreTestStatisticIntensityActivePort::setup() {
    HailstoneEvent* ev = new HailstoneEvent(seed_, 0);
    self_link_->send(ev);
}

void coreTestStatisticIntensityActivePort::handleEvent(Event *ev) {
    HailstoneEvent* hev = dynamic_cast<HailstoneEvent*>(ev);
    if (hev->n() != 1){
        int newN = ((hev->n()%2)==0) ? hev->n() / 2 : 3*hev->n() + 1;
        HailstoneEvent* new_ev = new HailstoneEvent(newN, hev->step() + 1);
        link_->send(new_ev);
    }

    num_events_->addData(1);
    observed_numbers_->addData(hev->n());
    traffic_intensity_->addData(ev->getDeliveryTime(), hev->n());
    delete hev;
}

coreTestStatisticIntensityActivePort::coreTestStatisticIntensityActivePort(ComponentId_t id, Params &params, int port, const std::vector<int> &num_init_events) :
    coreTestStatisticIntensityPort(id, params, port, num_init_events)
{
    if (params.contains("seed")){
        seed_ = params.find<int>("seed");
    } else {
        seed_ = 10*port + 1;
    }
    port_ = port;
    num_events_ = registerStatistic<int>("num_events", std::to_string(port));
    observed_numbers_ = registerStatistic<int>("observed_numbers", std::to_string(port));
    traffic_intensity_ = registerMultiStatistic<uint64_t,double>("traffic_intensity", std::to_string(port));
}

void coreTestStatisticIntensityInactivePort::handleEvent(Event *ev) {
    //do nothing, just drop the event and don't propagate
    delete ev;
}

coreTestStatisticIntensityInactivePort::coreTestStatisticIntensityInactivePort(ComponentId_t id, Params &params, int port, const std::vector<int> &num_init_events) :
    coreTestStatisticIntensityPort(id, params, port, num_init_events)
{
}

coreTestStatisticIntensityComponent::coreTestStatisticIntensityComponent(ComponentId_t cid, Params &params) : Component(cid) {
    std::vector<int> num_init_events;
    params.find_array("num_init_events", num_init_events);
    int num_ports = params.find<int>("num_ports");
    ports_.resize(num_ports);
    for (int p=0; p < num_ports; ++p){
        std::string port_name = std::string("port") + std::to_string(p);
        ports_[p] = loadUserSubComponent<coreTestStatisticIntensityPort>(port_name, ComponentInfo::SHARE_NONE,
                                                    p, num_init_events);
    }
}

void coreTestStatisticIntensityComponent::init(unsigned int phase){
    for (auto* port : ports_){
        port->init(phase);
    }
}

void coreTestStatisticIntensityComponent::complete(unsigned int phase){
}

void coreTestStatisticIntensityComponent::setup(){
    for (auto* port : ports_){
        port->setup();
    }
}

void coreTestStatisticIntensityComponent::finish(){
}

}
}
