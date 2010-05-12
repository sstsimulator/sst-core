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

#ifndef _QUEUE_H
/*---------------------------------------------------------+----------------\
| Simple byte queue for Psst                               | Chad D. Kersey |
+----------------------------------------------------------+----------------+
| The interface between the VM and the models is a simple queue of data     |
| containing a 1-byte opcode followed operands of the appropriate type, as  |
| specified in the function call.                                           |
\--------------------------------------------------------------------------*/
#define _QUEUE_H

#include <exception>
#include <sst_stdint.h>

class queue_overflow : public std::exception {
 private:
 public:
  virtual const char* what() { return "Queue overflow."; }
};

class queue_underflow : public std::exception {
 private:
 public:
  virtual const char* what() { return "Attempt to read from empty Queue."; }
};

/* Opcodes for the queues. */
enum Queue_opc {
  // Model => VM calls
  QOPC_CALLBACKREQUEST = 0,
  QOPC_SETIPC,

  // VM => Model calls (Events)
  QOPC_CALLBACK = 128,
  QOPC_MEMOP,
  QOPC_INSTRUCTION,
  QOPC_MAGICINST
};

class Queue {
 private:
  uint8_t *data, *limit;
  uint8_t *read_pos, *write_pos;
  size_t bytes, size;

 public:
  Queue(size_t len) : size(len), bytes(0) { 
    data = new uint8_t[size]; 
    limit = data + size;
    read_pos = write_pos = data;
  }

  ~Queue() { delete[] data; }

  template <class T> void put(T x) { 
    size_t max_write = size - bytes;
    if (sizeof(x) > max_write) throw queue_overflow();

    *(T *)(write_pos) = x;
    write_pos += sizeof(x);
    bytes += sizeof(x);
    if (write_pos >= limit) write_pos -= (limit - data);
  }
  
  template <class T> void get(T &x) {
    if (sizeof(x) > bytes) throw queue_underflow();
    
    x = *(T *)(read_pos);
    read_pos += sizeof(x);
    bytes -= sizeof(x);
    if (read_pos >= limit) read_pos -= (limit - data);
  }

  template <class T> void peek(T &x) {
    if (sizeof(x) > bytes) throw queue_underflow();
    
    x = *(T *)(read_pos);
  }

  void discard(size_t n) {
    if (n > bytes) n = bytes;
    bytes -= n;
    write_pos -= n;
    if (write_pos < data) write_pos += (limit - data);
  }

  int full() { return bytes == size; }
  int empty() { return bytes == 0; }
  size_t space() { return size - bytes; }
};
#endif
