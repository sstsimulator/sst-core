// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2005-2010, Sandia Corporation
// All rights reserved.
// Copyright (c) 2003-2005, University of Notre Dame
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef MEMORY_H
#define MEMORY_H

#include "global.h"
//#include "enkidu/enkidu.h"
#include <queue>
#include "specMem.h"
#include "fe_debug.h"
#include <sst/boost.h>
#include <boost/serialization/array.hpp>

//: Interface for memory objects
//!SEC:Framework
class memory_interface {
#if WANT_CHECKPOINT_SUPPORT
    BOOST_SERIALIZE {
    ;
  }
#endif
public:
  virtual ~memory_interface() {;}
  //: Read a byte
  virtual uint8 ReadMemory8(const simAddress, const bool)=0;
  //: Read a halfword
  virtual uint16 ReadMemory16(const simAddress, const bool)=0;
  //: Read a word (32-bits)
  virtual uint32 ReadMemory32(const simAddress, const bool)=0;
  //: Read a double word (64-bits)
  virtual uint64 ReadMemory64(const simAddress, const bool)=0;
  //: Write a byte
  virtual bool WriteMemory8(const simAddress, const uint8, const bool)=0;
  //: Write a halfword
  virtual bool WriteMemory16(const simAddress, const uint16, const bool)=0;
  //: Write a word (32-bits)
  virtual bool WriteMemory32(const simAddress, const uint32, const bool)=0;
  //: Write a double word (64-bits)
  virtual bool WriteMemory64(const simAddress, const uint64, const bool)=0;
  //: Get Full/Empty bits for an address
  virtual uint8 getFE(const simAddress)=0;
  //: Set Full/Empty bits for an address
  virtual void setFE(const simAddress, const uint8 FEValue)=0;
  //: Squash speculative state
  virtual void squashSpec()=0;
};
class base_memory;

struct  MemMapEntry {
#if WANT_CHECKPOINT_SUPPORT
  BOOST_SERIALIZE {
    ar & BOOST_SERIALIZATION_NVP(mem);
    ar & BOOST_SERIALIZATION_NVP(addr);
    ar & BOOST_SERIALIZATION_NVP(len);
    ar & BOOST_SERIALIZATION_NVP(offset);
    ar & BOOST_SERIALIZATION_NVP(writeLat);
  }
#endif
  memory_interface* mem;
  simAddress   addr;
  int          len;
  simAddress   offset;
  int          writeLat;
  //component    *comp;
};

typedef std::map<simAddress,MemMapEntry> MemMapByAddr;

enum { ReadOp=1, WriteOp };
enum { Size8, Size16, Size32, Size64 };

#define OpSize( op ) ( op >> 16 )
#define OpType( op ) ( op & 0xffff )

class MemAccess {
  friend class MemAccess_compare;

  public:

  static unsigned long counter;

  unsigned long long when;
  unsigned long number;
  simAddress         addr;
  uint32             value;
  bool               spec;
  int                size;
  MemMapEntry        *foo;

    MemAccess( unsigned long long _when,
                simAddress _addr,
                uint32 _value,
                bool _spec,
                int _size,
                MemMapEntry *_foo ) {
      when = _when;
      addr = _addr;
      value = _value;
      spec = _spec; 
      size = _size;
      foo = _foo; 
      number = counter++; 
    }
};

class MemAccess_compare {
  public:
    bool operator()(const MemAccess* x, const MemAccess* y) const
    { 
      if ( x->when > y->when ) return true;

      if ( x->when == y->when && x->number > y->number ) return true;
      
      return false;
    }
};



//: Abstract memory
//
// Generic memory storage object.
//
//!NOTE: no longer a component
//!SEC:Framework
class base_memory : public memory_interface
{
#if WANT_CHECKPOINT_SUPPORT
  BOOST_SERIALIZE {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( memory_interface );
    ar & BOOST_SERIALIZATION_NVP(defaultFEB);
    ar & BOOST_SERIALIZATION_NVP(PageArray);
    ar & BOOST_SERIALIZATION_NVP(FEArray);
    ar & BOOST_SERIALIZATION_NVP(PageSize);
    ar & BOOST_SERIALIZATION_NVP(PageShift);
    ar & BOOST_SERIALIZATION_NVP(PageMask);
    ar & BOOST_SERIALIZATION_NVP(sizeP);
    ar & BOOST_SERIALIZATION_NVP(numPagesP);
    ar & BOOST_SERIALIZATION_NVP(identP);
    /*ar & fdP;
    ar & backingFileNameP;
    ar & backingBitsP;
    ar & filePageBufP;
    ar & filePageAddrP;
    ar & gupsP;*/
    ar & BOOST_SERIALIZATION_NVP(memMapByAddr);
  }
#endif
  static uint8 defaultFEB;
  //: Array of Pages
  //
  // We store data by allocating page-sized chunks. These are stored
  // in the page array.
  //uint8  **PageArray;
  vector<vector<uint8>* > PageArray;
  //: Array of pages for full/empty bits
  // A seperate storage area is set up for Full/empty bits.
  //uint8  **FEArray;
  vector<vector<uint8>* > FEArray;
  //: Size of a page (in bytes)
  uint32 PageSize;
  //: Bits to shift an address to get page
  uint32 PageShift;
  //: Page address mask.
  uint32 PageMask;
  inline uint8 *GetPage (const simAddress sa);
  inline uint8  *GetFEPage(const simAddress);
  specMemory specMem;
 
  inline void readFileBackPage( const simAddress pageAddr, uint8 *bufAddr, int size ); 
  inline void writeFileBackPage( const simAddress pageAddr, uint8 *bufAddr, int size ); 

  ulong sizeP;
  uint  numPagesP;
  uint  identP;
  int   fdP;
  char  backingFileNameP[FILENAME_MAX];
  uint32 *backingBitsP;
  uint8 *filePageBufP;
  simAddress filePageAddrP;
  bool gupsP;

  MemMapByAddr memMapByAddr;

  MemMapEntry* FindMemByAddr( simAddress addr);
  priority_queue<MemAccess*,vector<MemAccess*>,MemAccess_compare> memWrite;


  //void ReadNotify( component *comp, simAddress addr, int size );
  //void WriteNotify( component *comp, simAddress addr, int size );
  
public:  
  base_memory(ulong size = 0x80000000 ,
	      uint pageSize = 0x4000 , uint ident = 0);
  static uint8 getDefaultFEB() { return defaultFEB; }
  //: Empty Deconstructor
  //
  //!NOTE: What was I thinking? We need to free up memory here.
  virtual ~base_memory() {;}
  uint8 _ReadMemory8(const simAddress, const bool);
  uint16 _ReadMemory16(const simAddress, const bool);
  uint32 _ReadMemory32(const simAddress, const bool);
  uint64 _ReadMemory64(const simAddress, const bool);
  bool _WriteMemory8(const simAddress, const uint8, const bool);
  bool _WriteMemory16(const simAddress, const uint16, const bool);
  bool _WriteMemory32(const simAddress, const uint32, const bool);
  bool _WriteMemory64(const simAddress, const uint64, const bool);
  virtual uint8 getFE(const simAddress);
  virtual void setFE(const simAddress, const uint8 FEValue);
  virtual void squashSpec() {specMem.squashSpec();}
  void clearMemory();
  bool hasPage(const simAddress);

  // The idea here is that address range X (starting at start, and continuing
  // for len bytes) is actually owned by some other component (i.e. some other
  // base_memory, mem). We can also say that we want component comp to get
  // notifications, if desired, of all memory operations on that base_memory.
  // writeLat allows you to specify a delay before the remote memory is
  // updated. farOffset allows you to translate the address by a given amount.
  /*int RegisterMem(memory_interface *mem, simAddress start, int len,
                simAddress farOffset, int writeLat, component *comp = NULL );
		void RegisterMemIF(string cfgStr, memory_interface *memIF, component *notify);*/
  bool LoadMem( const simAddress dest, void* const source, int Bytes);


#define REDIRECT_READ_FUNC_GET(S)		     \
  virtual uint##S ReadMemory##S(const simAddress sa,		      \
				const bool spec)		      \
  {								      \
    return _ReadMemory##S(sa,spec);				      \
  }
 
#define REDIRECT_WRITE_FUNC_GET(S)		   \
  virtual bool WriteMemory##S(const simAddress sa,			\
			      const uint##S Data, const bool spec)	\
  {									\
    return _WriteMemory##S(sa,Data,spec);				\
  } 

#define SST_DELAY(S)					   \
  bool delayWriteMemory##S( MemMapEntry *tmp,				\
					 const simAddress sa, const uint##S Data, const bool spec) \
  {									\
    simAddress addr =  tmp->offset + (sa - tmp->addr);			\
									\
    DPRINT(V1,"sa=%#x new-addr=%#x  value=%x\n",(uint)sa,(uint)addr,(int)Data); \
									\
    bool ret = tmp->mem->WriteMemory##S( addr, Data, spec );		\
    if ( tmp->comp ) {							\
      WriteNotify( tmp->comp, addr, Size##S );				\
    }									\
    return ret;								\
  }

  REDIRECT_READ_FUNC_GET(8)
  REDIRECT_READ_FUNC_GET(16)
  REDIRECT_READ_FUNC_GET(32)
  REDIRECT_READ_FUNC_GET(64)

  REDIRECT_WRITE_FUNC_GET(8)
  REDIRECT_WRITE_FUNC_GET(16)
  REDIRECT_WRITE_FUNC_GET(32)
  REDIRECT_WRITE_FUNC_GET(64)

  //SST_DELAY(8)
  //SST_DELAY(16)
  //SST_DELAY(32)
#undef DELAY

 void setup();
 void finish();
 void preTic();

};

typedef enum {
  GlobalTextMem = 1,
  GlobalDataMem = 2,
  LocalDynamic = 3,
  GlobalDynamic = 4,
  AddressError = 0,
  MemTypes = 5
} memoryAccType;

#include "mallocSysCall.h"

//: Memory component
//
// This is the base class for processors, because it is a component it
// can receive parcels.
//
//!SEC:Framework
class memory : public memory_interface {
#if WANT_CHECKPOINT_SUPPORT
  BOOST_SERIALIZE {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( memory_interface );
    ar & BOOST_SERIALIZATION_NVP(myMem);
    /*ar & globalDynamic;
    ar & localDynamic;
    ar & segRange;
    ar & segName;*/
    ar & BOOST_SERIALIZATION_NVP(madeMemory);
  }
#endif
  base_memory *myMem;
  static vmRegionAlloc globalDynamic;
  static localRegionAlloc localDynamic;

public:
  static simAddress segRange[MemTypes][2];
  static const char *segName[MemTypes];

  bool madeMemory;

  base_memory *getBaseMem() {return myMem;}

  //: Constructor
  //
  // Pass in a base memory, or don't (if it should create its own).
  memory() : myMem(new base_memory()), madeMemory(1) {}
  memory(base_memory *bm) : myMem(bm), madeMemory(0)  {
      if (bm == NULL) {
	  myMem = new base_memory();
	  madeMemory = 1;
      }
  }

  virtual ~memory() {
    if (myMem && madeMemory) {
      delete myMem;
    }
  }

  static memoryAccType getAccType(simAddress sa);

  virtual uint8 getFE(const simAddress sa) {return myMem->getFE(sa);}
  virtual void setFE(const simAddress sa, const uint8 FEValue) {
    myMem->setFE(sa, FEValue);
  }
  virtual void squashSpec() {myMem->squashSpec();}

  virtual uint8 ReadMemory8(const simAddress sa, const bool s);
  virtual bool WriteMemory8(const simAddress sa, const uint8 d, const bool s);
  virtual uint16 ReadMemory16(const simAddress sa, const bool s);
  virtual bool WriteMemory16(const simAddress sa, const uint16 d, const bool s);
  virtual uint32 ReadMemory32(const simAddress sa, const bool s);
  virtual bool WriteMemory32(const simAddress sa, const uint32 d, const bool s);
  virtual uint64 ReadMemory64(const simAddress sa, const bool s);
  virtual bool WriteMemory64(const simAddress sa, const uint64 d, const bool s);

  static simAddress globalAllocate (unsigned int size);
  static simAddress localAllocateNearAddr (unsigned int size, simAddress addr);
  static simAddress localAllocateAtID (unsigned int size, unsigned int ID);
  static unsigned int memFree(simAddress addr, unsigned int size);

  static void setUpLocalDistribution ( uint hashShift, uint locCount) {
    localDynamic.setup(hashShift, locCount);
  }
  static void addLocalID (processor *p, uint locID) {
    localDynamic.addLoc(p, locID);
  }
  static int getLocalID (processor *p) {
    return localDynamic.getLoc(p);
  }
  static int whichLoc (simAddress sa) {
    return localDynamic.whichLoc(sa);
  }
  static uint maxLocalChunk() {
    return localDynamic.stride();
  }
  static uint numLocales() {
    return localDynamic.locs();
  }
  static simAddress addrOnLoc (uint i) {
    return localDynamic.addrOnLoc(i);
  }
};

#endif
