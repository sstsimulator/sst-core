// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2007-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SSB_DMA_FAKEINST_H
#define SSB_DMA_FAKEINST_H

#include "instruction.h"

//: "fake" DMA instruction
//
// Used to send memory requests to memory
//
//!SEC:NIC_Model
class fakeDMAInstruction : public instruction {
  static int negOne;
  instType _op;
  simAddress _addr;
  simPID _pid;
  instState _state;
public:
  fakeDMAInstruction() {;}
  void init(instType o, simAddress addr, simPID p) {
    _op = o; _addr = addr; _pid = p;
    _state = ISSUED;
  }
  void init(instType o, simAddress addr, simPID p, instState s) {
    init(o, addr, p); 
    _state = s;
  }
  bool fetch(processor*) {printf("Fake Instr!\n"); return 0;};
  bool issue(processor*) {printf("Fake Instr!\n"); return 0;};
  bool commit(processor*) {printf("Fake Instr!\n"); return 0;}; 
  simRegister NPC() const {printf("Fake Instr!\n"); return 0;};
  simRegister TPC() const {printf("Fake Instr!\n"); return 0;};
  bool issue(processor *p, const bool) {printf("Fake Instr!\n"); return 0;}; 
  bool commit(processor *p, const bool) {printf("Fake Instr!\n"); return 0;};
  int fu() const {printf("Fake Instr!\n"); return 0;};
  int specificOp() const {printf("Fake Instr!\n"); return 0;};
  const int* outDeps() const {printf("Fake Instr!\n"); return &negOne;};
  const int* inDeps() const {printf("Fake Instr!\n"); return &negOne;};
  bool isReturn() const {printf("Fake Instr!\n"); return 0;};
  instState state() const {return _state;}
  instType op() const {return _op;}
  simAddress PC() const {printf("Fake Instr!\n"); return 0;};
  simAddress memEA() const {return _addr;}
  exceptType exception() const {printf("Fake Instr!\n"); return NO_EXCEPTION;};
  simAddress moveToTarget() const {printf("Fake Instr!\n"); return 0;};
  simAddress febTarget() const {printf("Fake Instr!\n"); return 0;};
  simPID pid() const {return _pid;}
};


#endif
