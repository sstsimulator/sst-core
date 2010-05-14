// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef __NETWORK_H
#define __NETWORK_H


//test-router

#include <vector>
#include <fstream>
#include <sst/component.h>
using namespace SST;
#include <sst_stdint.h>
typedef uint32_t uint32;
typedef unsigned int uint;

const uint32 HDR_SIZE =  8/sizeof(int);
const uint32 PKT_SIZE =  64/sizeof(int);

const int MaxPacketSize = HDR_SIZE + PKT_SIZE;

using namespace std;

#define NIC_VC_0 0
#define NIC_VC_1 2

#define NIC_2_RTR_VC(vcc) ( vcc == 0 ? NIC_VC_0 : NIC_VC_1 )
#define RTR_2_NIC_VC(vcc) ( vcc/2 )

#define LINK_TX 0
#define LINK_RX 1

#define ROUTER_NUM_LINKS 6
#define ROUTER_NUM_VCS 4

#define ROUTER_HOST_OUTQS 1
#define ROUTER_HOST_INQS 1
#define ROUTER_HOST_PORT 6

#define ROUTER_NUM_OUTQS (ROUTER_NUM_LINKS + ROUTER_HOST_OUTQS)
#define ROUTER_NUM_INQS (ROUTER_NUM_LINKS + ROUTER_HOST_INQS)

#define DIST_POS(A,B,dimSize) (A < B) ? B - A : dimSize - (A - B)
#define DIST_NEG(A,B,dimSize) (A < B) ? dimSize - ( B - A ) : A - B

#define LINK_VC0 0
#define LINK_VC1 1
#define LINK_VC2 2
#define LINK_VC3 3
#define LINK_CTRL_NUL 4

#define LINK_NUL_STATE 0
#define LINK_CTRL_SENT 1
#define LINK_ACK_SENT 2
#define LINK_DATA_SENT 3

#define LINK_POS_X 0
#define LINK_NEG_X 1
#define LINK_POS_Y 2
#define LINK_NEG_Y 3
#define LINK_POS_Z 4
#define LINK_NEG_Z 5

static const char* LinkNames[] = {
    "xPos",
    "xNeg",
    "yPos",
    "yNeg",
    "zPos",
    "zNeg",
    "nic",
};

#define REVERSE_DIR(dir, opp) switch (dir) { \
  case LINK_POS_X: opp = LINK_NEG_X; break;    \
  case LINK_POS_Y: opp = LINK_NEG_Y; break;    \
  case LINK_POS_Z: opp = LINK_NEG_Z; break;    \
  case LINK_NEG_X: opp = LINK_POS_X; break;    \
  case LINK_NEG_Y: opp = LINK_POS_Y; break;    \
  case LINK_NEG_Z: opp = LINK_POS_Z; break; }


static  inline uint32 CalcNumFlits( int nwords ) {
    int numFlits = nwords % 2 ? nwords+1 : nwords; // adjust for flits
    numFlits >>= 1;                                // words to flits
    return numFlits;
}

class Network {
    int _xDimSize, _yDimSize, _zDimSize;
    int _size;

public:
    Network( Component::Params_t );
    int xDimSize () {
        return _xDimSize;
    }
    int yDimSize () {
        return _yDimSize;
    }
    int zDimSize () {
        return _zDimSize;
    }
    int size() {
        return  _size;
    }
};


//: Network Flit
//
//!SEC:EDSim
class networkFlit {
    enum flit_t { HEAD, BODY, NUL};
    flit_t flitType;
    static const int size=8;     //8 bytes
};
//: Network Packet
//
//!SEC:EDSim
class networkPacket {

    int _destNum;
    int _sourceNum;
    uint _sizeInFlits;
    int _vc;
    int _link;


public:
    int &vc () {
        return _vc;
    }
    int &link () {
        return _link;
    }

    int &srcNum() {
        return _sourceNum;
    }
    int srcNum() const {
        return _sourceNum;
    }
    int &destNum() {
        return _destNum;
    }
    int destNum() const {
        return _destNum;
    }

    uint &sizeInFlits() {
        return _sizeInFlits;
    }
    uint sizeInFlits() const {
        return _sizeInFlits;
    }

    uint32 payload[HDR_SIZE+PKT_SIZE];
};

#include <sst/compEvent.h>
#include <sst_stdint.h>

struct RtrEvent : public CompEvent {
    typedef enum { Credit, Packet } msgType_t;
    int             type;
    union {
        networkPacket   packet;
        struct {
            uint32_t        num;
            int             vc;
        } credit;
    } u;
};

#endif
