// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/testElements/coreTest_SubComponent.h"

#include "sst/core/testElements/coreTest_Message.h"

#include "sst/core/clock.h"
#include "sst/core/output.h"
#include "sst/core/timeConverter.h"

using namespace SST;
using namespace SST::CoreTestSubComponent;

/************************************************************************
 *
 *    SubComponentLoader
 *
 ***********************************************************************/

SubComponentLoader::SubComponentLoader(ComponentId_t id, Params& params) : Component(id)
{
    std::string freq = params.find<std::string>("clock", "1GHz");

    registerClock(freq, new Clock::Handler2<SubComponentLoader, &SubComponentLoader::tick>(this));

    std::string unnamed_sub  = params.find<std::string>("unnamed_subcomponent", "");
    int         num_subcomps = params.find<int>("num_subcomps", 1);

    if ( unnamed_sub != "" ) {
        for ( int i = 0; i < num_subcomps; ++i ) {
            params.insert("port_name", std::string("port") + std::to_string(i));
            params.insert("verbose", params.find<std::string>("verbose", "0"));
            SubCompInterface* sci = loadAnonymousSubComponent<SubCompInterface>(
                unnamed_sub, "mySubComp", i, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, params);
            subComps.push_back(sci);
        }
    }
    else {
        SubComponentSlotInfo* info = getSubComponentSlotInfo("mySubComp");
        if ( !info ) {
            Output::getDefaultObject().fatal(
                CALL_INFO, -1, "Must specify at least one SubComponent for slot mySubComp.\n");
        }

        info->createAll<SubCompInterface>(subComps, ComponentInfo::SHARE_STATS);
    }

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

bool
SubComponentLoader::tick(Cycle_t cyc)
{
    for ( auto sub : subComps ) {
        sub->clock(cyc);
    }
    return false;
}

/************************************************************************
 *
 *    SubCompSlot
 *
 ***********************************************************************/
SubCompSlot::SubCompSlot(ComponentId_t id, Params& params) : SubCompSlotInterface(id)
{
    std::string unnamed_sub  = params.find<std::string>("unnamed_subcomponent", "");
    int         num_subcomps = params.find<int>("num_subcomps", 1);

    if ( unnamed_sub != "" ) {
        for ( int i = 0; i < num_subcomps; ++i ) {
            params.insert("port_name", std::string("slot_port") + std::to_string(i));
            params.insert("verbose", params.find<std::string>("verbose", "0"));
            SubCompInterface* sci = loadAnonymousSubComponent<SubCompInterface>(
                unnamed_sub, "mySubCompSlot", i, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, params);
            subComps.push_back(sci);
        }
    }
    else {
        SubComponentSlotInfo* info = getSubComponentSlotInfo("mySubCompSlot");
        if ( !info ) {
            Output::getDefaultObject().fatal(
                CALL_INFO, -1, "Must specify at least one SubComponent for slot mySubComp.\n");
        }

        info->createAll<SubCompInterface>(subComps, ComponentInfo::SHARE_STATS);
    }
}

void
SubCompSlot::clock(Cycle_t cyc)
{
    for ( auto sub : subComps ) {
        sub->clock(cyc);
    }
}

/************************************************************************
 *
 *    SubCompSender
 *
 ***********************************************************************/
SubCompSender::SubCompSender(ComponentId_t id, Params& params) : SubCompSendRecvInterface(id)
{
    // Determine if I'm loading as a named or unmamed SubComponent
    std::string port_name;
    if ( isUser() ) { port_name = "sendPort"; }
    else
        port_name = params.find<std::string>("port_name");

    registerTimeBase("2GHz", true);
    link = configureLink(port_name);
    if ( !link ) {
        Output::getDefaultObject().fatal(CALL_INFO, -1, "Failed to configure port %s\n", port_name.c_str());
    }

    nMsgSent = registerStatistic<uint32_t>("numSent", "");
    if ( isStatisticShared("totalSent") ) { totalMsgSent = registerStatistic<uint32_t>("totalSent", ""); }
    else {
        totalMsgSent = NULL;
    }
    nToSend = params.find<uint32_t>("sendCount", 10);
    out     = new SST::Output("", params.find<uint32_t>("verbose", 0), 0, SST::Output::output_location_t::STDOUT);
}

void
SubCompSender::clock(Cycle_t cyc)
{
    if ( nToSend == 0 ) return;

    if ( (cyc % 64) == 0 ) {
        link->send(new CoreTestMessageGeneratorComponent::coreTestMessage());
        if ( nMsgSent ) nMsgSent->addData(1);
        if ( totalMsgSent ) totalMsgSent->addData(1);
        nToSend--;
        out->verbose(CALL_INFO, 1, 0, "Sent an event, %d more to send\n", nToSend);
    }
}

/************************************************************************
 *
 *    SubCompReceiver
 *
 ***********************************************************************/
SubCompReceiver::SubCompReceiver(ComponentId_t id, Params& params) : SubCompSendRecvInterface(id)
{
    // Determine if I'm loading as a named or unmamed SubComponent
    std::string port_name;
    if ( isUser() )
        port_name = "recvPort";
    else
        port_name = params.find<std::string>("port_name");

    link = configureLink(port_name, new Event::Handler2<SubCompReceiver, &SubCompReceiver::handleEvent>(this));
    if ( !link ) { Output::getDefaultObject().fatal(CALL_INFO, -1, "Failed to configure port 'recvPort'\n"); }
    // registerTimeBase("1GHz", true);
    nMsgReceived = registerStatistic<uint32_t>("numRecv", "");
    out          = new SST::Output("", params.find<uint32_t>("verbose", 0), 0, SST::Output::output_location_t::STDOUT);
}

void
SubCompReceiver::clock(Cycle_t UNUSED(cyc))
{
    /* Do nothing */
}

void
SubCompReceiver::handleEvent(Event* ev)
{
    out->verbose(CALL_INFO, 1, 0, "Got an event\n");
    if ( nMsgReceived ) nMsgReceived->addData(1);
    delete ev;
}
