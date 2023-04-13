// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//
#include "sst_config.h"

#include "sst/core/interfaces/simpleNetwork.h"

#include "sst/core/objectComms.h"

using namespace std;

namespace SST {
namespace Interfaces {

const SimpleNetwork::nid_t SimpleNetwork::INIT_BROADCAST_ADDR = 0xffffffffffffffffl;

void
SimpleNetwork::sendUntimedData(Request* req)
{
    if ( delegate_send ) {
        fatal(
            CALL_INFO, 1,
            "ERROR: Using SimpleNetwork implementation that doens't implement either sendUntimedData() or "
            "sendInitData()\n");
    }
    delegate_send = true;
    sendInitData(req);
    delegate_send = false;
}

SimpleNetwork::Request*
SimpleNetwork::recvUntimedData()
{
    if ( delegate_recv ) {
        fatal(
            CALL_INFO, 1,
            "ERROR: Using SimpleNetwork implementation that doens't implement either recvUntimedData() or "
            "recvInitData()\n");
    }
    delegate_recv = true;
    auto* ret     = recvInitData();
    delegate_recv = false;
    return ret;
}

void
SimpleNetwork::sendInitData(Request* UNUSED(req))
{
    if ( delegate_send ) {
        fatal(
            CALL_INFO, 1,
            "ERROR: Using SimpleNetwork implementation that doens't implement either sendUntimedData() or "
            "sendInitData()\n");
    }
    delegate_send = true;
    sendUntimedData(req);
    delegate_send = false;
}

SimpleNetwork::Request*
SimpleNetwork::recvInitData()
{
    if ( delegate_recv ) {
        fatal(
            CALL_INFO, 1,
            "ERROR: Using SimpleNetwork implementation that doens't implement either recvUntimedData() or "
            "recvInitData()\n");
    }
    delegate_recv = true;
    auto* ret     = recvUntimedData();
    delegate_recv = false;
    return ret;
}

} // namespace Interfaces
} // namespace SST
