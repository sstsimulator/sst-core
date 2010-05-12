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

#include <stdio.h>
#include "ppcPimCalls.h"

const int msgSz = 7;
unsigned int buf[msgSz];

void readFunc(void *arg) {
  int source = -1;
  int i;

  //wait for an incoming message
  while (source == -1) {
    source = PIM_readSpecial(PIM_CMD_NET_CHECK_INCOMING);
  }

  // handle the messge
  if (source == 0) {
    int dmaChan = PIM_readSpecial(PIM_CMD_DMA_INIT, 
				  PIM_ADDR_NET_IN, //network memory
				  source, // location
				  msgSz << 2); //size

    if (dmaChan != -1) {
      // start DMA
      PIM_writeSpecial(PIM_CMD_DMA_START,
		       dmaChan, //DMA channel
		       PIM_ADDR_LOCAL, // output
		       (unsigned int)buf); // buffer location

      // wait for DMA to complete      
      PIM_dmaStatus dmaStat = DMA_WORKING;
      do {
	dmaStat = (PIM_dmaStatus)PIM_readSpecial(PIM_CMD_DMA_STATUS, dmaChan);
      } while (dmaStat == DMA_WORKING);

      PIM_quickPrint(2, 2, 0);

      PIM_switchAddrMode(PIM_ADDR_LOCAL);
      for (i = 0; i < msgSz; ++i) 
	PIM_quickPrint(2, 2, buf[i]);

    } else {
      PIM_quickPrint(9, source, 7);
    }
  } else {
    PIM_quickPrint(9, source, 8);
  }
}

void writeFunc(void *arg) {
  int bufNum;
  int i,j;

  // get an outgoing buffer
  bufNum = PIM_readSpecial(PIM_CMD_NET_CHECK_OUTGOING);
  PIM_quickPrint(2, 2, bufNum);
  if (bufNum != -1) {
    // set dest
    PIM_writeSpecial(PIM_CMD_NET_SET_OUTGOING_DEST, bufNum, 1);
    // copy message
    int dmaChan = PIM_readSpecial(PIM_CMD_DMA_INIT, 
				  PIM_ADDR_NONLOCAL, //mainProc memory
				  (unsigned int)buf, // location
				  msgSz << 2); //size
    if (dmaChan != -1) {
      // start DMA
      PIM_writeSpecial(PIM_CMD_DMA_START,
		       dmaChan, //DMA channel
		       PIM_ADDR_NET_OUT, // output
		       bufNum); // buffer location

      // wait for DMA to complete      
      PIM_dmaStatus dmaStat = DMA_WORKING;
      do {
	dmaStat = (PIM_dmaStatus)PIM_readSpecial(PIM_CMD_DMA_STATUS, dmaChan);
      } while (dmaStat == DMA_WORKING);

      PIM_quickPrint(2, 2, 0);
      // the DMA sends when it is done
      //PIM_writeSpecial(PIM_CMD_NET_SEND_DONE, 
      //bufNum);
    } else {
      PIM_quickPrint(9, 9, 8);
    }
  } else {
    PIM_quickPrint(9, 9, 9);
  }
}

int b = 0;

class foo {
public:
  foo() {printf("hi\n");}
};

foo *bar;


int main() {
  int rank;
  int i;

  bar = new foo();

  rank = PIM_readSpecial(PIM_CMD_PROC_NUM);
  PIM_quickPrint(rank, rank, rank);
  printf("start %d\n", rank);
  fflush(stdout);

  if (rank == 0) {
    for (i = 0; i < msgSz; ++i) 
      buf[i] = i;
    PIM_spawnToCoProc(PIM_NIC, (void*)writeFunc, 0);
  } else {
    PIM_spawnToCoProc(PIM_NIC, (void*)readFunc, 0);
  }

  // wait loop
  for (i = 0; i < 30000; ++i) {
    b++;
  }

  return 0;
}
