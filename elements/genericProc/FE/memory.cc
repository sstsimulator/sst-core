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


#include "memory.h"
//#include "configuration.h"
#include <string.h>
#include <strings.h>
//#include "fe_debug.h"
#include <fcntl.h>



#define INVALID_FD -1
// Default FEB value
uint8 base_memory::defaultFEB = 0;
static uint32 FEBSplat = 0;

unsigned long MemAccess::counter = 0;

//: FEB granularity
// Bits to shift an address to get the FEB address. Currently set to
// 2, so we get a FEB per word.
static const uint32 FEShift=2; /* per word */

//: Initialize a memory object
//
// Primarily this works by allocating the PageArray and FEArray. It
// also sets up the various page shifts and masks. Currently, it
// allocates room for a 2 gig address space. Uses 16K pages (note,
// these may have nothing to do with the pagesize of the system, they
// are just the units that memory is allocated in
// internally. (translation: don't tell the architects about this page
// size, its just for simulation writers to care about))
//
// PageArray is an array which points to 16K pages. These pages are
// allocated on an as-needed basis so we don't have to malloc 2Gig up
// front for each memory object.
base_memory::base_memory(ulong size, uint pageSize, uint ident) :
  specMem(this), sizeP(size), identP(ident), fdP(INVALID_FD), gupsP(false)
{
  /*int in = configuration::getValue(cfgstr + ":defaultFEB");
  if (in > 0) {
    defaultFEB = in;
    FEBSplat = defaultFEB + (defaultFEB<<8) + (defaultFEB<<16) 
      + (defaultFEB<<24);
    printf("Default FEB set to %d (splot=%x)\n", in, FEBSplat);
    }*/
  defaultFEB = 0;

  numPagesP = sizeP/pageSize; 
  const uint32 inPageSize = pageSize;

  DPRINT(0,"numPages=%x pageSize=%x size=%lx ident=%d\n",
	 numPagesP,pageSize,sizeP,(int)identP);
  uint32 Index = inPageSize>>1;
  uint32 Index2 = 1;   
  uint32 Index3 = 1;
  while(Index) {
    ++Index2;
    Index3 = (Index3 << 1)|1;
    Index = Index >> 1;
  }
  PageShift = Index2-1;
  PageMask = Index3 >> 1;
  PageSize = (1<<(PageShift));

  /*int tmp = configuration::getValue( cfgstr + ":fileBacked" );

  if ( ! _config_error && tmp == 1 ) {

    sprintf(backingFileNameP,"/tmp/base_memory-%d.%d", getpid(),this);
    INFO("opening backing file %s\n",backingFileNameP);
    if ( ( fdP = open(backingFileNameP,O_CREAT|O_TRUNC|O_RDWR,0600) ) == -1 ) {
      char tmp2[64+FILENAME_MAX];
      sprintf(tmp2,"can't open base_memoryfile %s",backingFileNameP);
      perror(tmp2);
      exit(0);
    }
    backingBitsP = new uint32[numPagesP/sizeof(uint32)*8];
    bzero( backingBitsP, numPagesP/sizeof(uint32)*8 );
    
    filePageBufP = new uint8[pageSize];
    filePageAddrP = 0;

    } else {*/
    
  //PageArray = new uint8 *[numPagesP];
  PageArray = vector<vector<uint8>* >(numPagesP);
  FEArray  = vector<vector<uint8>* >(numPagesP);
  Index = 0;
  for(Index=0;Index<numPagesP;Index++) {
    PageArray[Index] = NULL;
    FEArray[Index] = NULL;
  }
  //}

  /*tmp = configuration::getValue( cfgstr + ":gupsInit" );

  if ( ! _config_error && tmp == 1 ) {
    INFO("using gups initialization\n");
    gupsP = true;
    }*/
}

void base_memory::setup() {;}
void base_memory::finish(){
  if ( fdP != INVALID_FD ) {
    INFO("removing backing file %s\n",backingFileNameP);
    close(fdP);
    unlink(backingFileNameP);
  }
}

void base_memory::preTic()
{
  /*while( memWrite.size() > 0 &&
        ( (memWrite.top())->when <= TimeStamp() )) {

#if 0
    PRINTF("time=%lld addr=%#lx value=%#lx size=%d\n", TimeStamp(),
                             (memWrite.top())->addr,
                             (uint32)(memWrite.top())->value,
                             (memWrite.top())->size);
#endif

    if (  (memWrite.top())->size == Size8 ) {
      delayWriteMemory8(      (memWrite.top())->foo,
                             (memWrite.top())->addr,
                             (uint8)(memWrite.top())->value,
                             (memWrite.top())->spec);
    } else if ( (memWrite.top())->size == Size16 ) {
      delayWriteMemory16( (memWrite.top())->foo,
                             (memWrite.top())->addr,
                             (uint16)(memWrite.top())->value,
                             (memWrite.top())->spec);
    } else if ( (memWrite.top())->size == Size32 ) {
      delayWriteMemory32( (memWrite.top())->foo,
                             (memWrite.top())->addr,
                             (uint32)(memWrite.top())->value,
                             (memWrite.top())->spec);
    } else {
      ERROR("Bad size %#x\n", (memWrite.top())->size);
    }
    delete memWrite.top();
    memWrite.pop();
    }*/
}

void base_memory::clearMemory() {
 if ( fdP == INVALID_FD ) {
  for(uint32 index=0; index<numPagesP;++index) {
    if(PageArray[index]) {
      delete PageArray[index];
      PageArray[index]=0;
    }
    if(FEArray[index]) {
      delete FEArray[index];
      FEArray[index] = 0;
    }
  }
 } else {
   ERROR("not supported for file back memory\n");
 }
}

//: Return if a page exists
//
// Used to see if a PC is valid
bool base_memory::hasPage(const simAddress sa) {
 if ( fdP == INVALID_FD ) {
  simAddress saCp = sa & 0x7fffffff;
  uint32 Index = (saCp >> PageShift);
  if(PageArray[Index]) {
    return 1;
  } else {
    return 0;
  }
 } else {
   ERROR("not supported for file back memory\n");
 }
}

//: Return pointer to a page
//
// Returns a host pointer to the simulation page containting the
// requested address. If the page has not been accessed, it will be
// allocated and zeroed
uint8 *base_memory::GetPage (const simAddress sa)
{
  uint8 *page = NULL;
  bool  initFlag = false;
  simAddress saCp = sa & 0x7fffffff;
  uint32        Index = (saCp >> PageShift);

  if ( fdP == INVALID_FD ) {
    if(!PageArray[Index]) {
      //PageArray[Index] = new uint8[PageSize];
      PageArray[Index] = new vector<uint8>(PageSize);
      initFlag = true;
    }
    //page = PageArray[Index];
    page = &((PageArray[Index])->operator[](0));
  } else {
    simAddress pageAddr = sa & ~(PageSize - 1);
    int wordPos = Index/32;
    int bitPos = Index%32;

    DPRINT(1,"ident=%d pageAddr %#x sa=%#x Index=%d wordPos=%d bitPos=%d\n",
                      identP-1,pageAddr,sa,Index,wordPos,bitPos);

    if ( ! (backingBitsP[wordPos] & ( 1 << bitPos ))  ) {
      DPRINT(0,"page fault %#lx wordPos=%d bitPos=%d\n",
						(unsigned long) pageAddr,wordPos,bitPos);
      backingBitsP[wordPos] |=  1 << bitPos;
      initFlag = true;
    }

    if ( pageAddr != filePageAddrP ) {
      DPRINT(0,"page evict old %#lx new %#lx\n",
            (unsigned long) filePageAddrP, (unsigned long) pageAddr);
      writeFileBackPage( filePageAddrP, filePageBufP, PageSize );
      filePageAddrP = pageAddr;
      if ( ! initFlag ) {
        readFileBackPage( filePageAddrP, filePageBufP, PageSize );
      } 
    }

    page = filePageBufP;
  }
  if ( initFlag == true ) {
    if ( ! gupsP ) {
      memset(page, 0x00, PageSize);
    } else {
      
      int tmp = (identP - 1) * sizeP/sizeof(int) + sa / sizeof(int);

      DPRINT(0,"sa=%#lx %#lx %#lx\n",(unsigned long) sa,
            (unsigned long) sizeP, (unsigned long) tmp);

      for (unsigned int i = 0; i < PageSize/sizeof(int); i++ ) {
       ((int*)page)[i] = tmp + i; 
      }
    }
  }
  return(page);
}


void base_memory::writeFileBackPage( const simAddress pageAddr, 
		uint8 *bufAddr, int size )
{
  DPRINT(1,"pageAddr=%#lx bufAddr=%p size=%d\n",
        (unsigned long) pageAddr,bufAddr,size);
  if ( lseek( fdP, pageAddr , SEEK_SET ) != pageAddr ) {
    perror("");
    ERROR("can't lseek to address %#lx\n", (unsigned long) pageAddr);
  }
  assert(size > 0);
  assert((unsigned)size == PageSize);
  if ( write( fdP, bufAddr, size ) != size ) {
    perror("");
    ERROR("can't write address %#lx\n", (unsigned long) pageAddr);
  }
}

void base_memory::readFileBackPage( const simAddress pageAddr,
		uint8 *bufAddr, int size )
{
  DPRINT(1,"pageAddr=%#lx bufAddr=%p size=%d\n",
                    (unsigned long) pageAddr,bufAddr,size);
  if ( lseek( fdP, pageAddr, SEEK_SET ) != pageAddr ) {
    perror("");
    ERROR("can't lseek to address %#lx\n",(unsigned long) pageAddr);
  }
  assert(size > 0);
  assert((unsigned)size == PageSize);
  if ( read( fdP, bufAddr, size ) != size) {
    perror("");
    ERROR("can't read address %#lx\n",(unsigned long) pageAddr);
  }
}

//: Return pointer to a page of Full/Empty bits
//
// Operation similar to that of base_memory::GetPage()
inline uint8 *base_memory::GetFEPage(const simAddress a)
{
  simAddress saCp = a & 0x7fffffff;
  uint32        Index = (saCp >> PageShift);
  if (!FEArray[Index]) {
    // get a new FEBPage and initialize it. 
    FEArray[Index] = new vector<uint8>(PageSize>>FEShift);
    memset(&((FEArray[Index])->operator[](0)), FEBSplat, PageSize>>FEShift);
  }
  vector<uint8> *page = FEArray[Index];
  uint8 *pageStart = &(page->operator[](0));
  return pageStart;
}

//: Get the FE bits for a given address
uint8 base_memory::getFE(const simAddress a)
{
  uint8 *Page = GetFEPage(a);
  return(Page[(a&PageMask)>>FEShift]);
}

//: Set the FE bits for a given address
void base_memory::setFE(const simAddress a, const uint8 FEValue)
{
  uint8 *Page = GetFEPage(a);
  Page[(a&PageMask)>>FEShift] = FEValue;
}

//:Read a byte
uint8 base_memory::_ReadMemory8(const simAddress sa, const bool spec)
{
  if (spec) {
    return specMem.readSpec8(sa);
  } else {
    uint8 *Page = GetPage(sa);
    if(!Page)     return(0xff);
    
    return(Page[sa&PageMask]);
  }
}

//:Read 2 bytes
uint16 base_memory::_ReadMemory16(const simAddress sa, const bool spec)
{
  if (spec) {
    return specMem.readSpec16(sa);
  } else {
    uint8 *Page = GetPage(sa);
    if(!Page)     return(0xffff);

    unsigned int index = sa&PageMask;
    if (index + 2 > PageSize) {
	uint8 *Page2 = GetPage(sa+2);
	uint16 Temp;
	memcpy(&Temp, &Page[index], 1);
	memcpy(((unsigned char *)(&Temp))+1, Page2, 1);
	return Temp;
    } else {
	Page = &Page[index];
	uint16 *TempPtr = (uint16*)Page;
        uint16 Temp;
        memcpy(&Temp, TempPtr, sizeof(uint16));
	return(Temp);
    }
  }
}

//:Read 4 bytes
uint32 base_memory::_ReadMemory32(const simAddress sa, const bool spec)
{
  if (spec) {
    return specMem.readSpec32(sa);
  } else {
    uint8 *Page = GetPage(sa);
    if(!Page) {
	return(0xffffffff);
    }

    unsigned int index = sa&PageMask;
    if (index + 4 > PageSize) {
	uint8 *Page2 = GetPage(sa+4);
	uint32 Temp;
	memcpy(&Temp, &Page[index], PageSize-index);
	memcpy(((unsigned char*)(&Temp))+(PageSize-index), 
		Page2, 4-(PageSize-index));
	return(Temp);
    } else {
	Page = &Page[index];
	uint32 *TempPtr = (uint32*)Page;
        uint32 Temp;
        memcpy(&Temp, TempPtr, sizeof(uint32));
        return Temp;
    }
  }
}

uint64 base_memory::_ReadMemory64(const simAddress sa, const bool spec)
{
  if (spec) {
    return specMem.readSpec32(sa);
  } else {
    uint8 *Page = GetPage(sa);
    if(!Page) {
	return(0xffffffff);
    }

    unsigned int index = sa&PageMask;
    if (index + 8 > PageSize) {
	uint8 *Page2 = GetPage(sa+8);
	uint64 Temp;
	memcpy(&Temp, &Page[index], PageSize-index);
	memcpy(((unsigned char*)(&Temp))+(PageSize-index), 
		Page2, 8-(PageSize-index));
	return(Temp);
    } else {
	Page = &Page[index];
	uint64 *TempPtr = (uint64*)Page;
        uint64 Temp;
        memcpy(&Temp, TempPtr, sizeof(uint64));
        return Temp;
    }
  }
}

//:Write a byte
bool base_memory::_WriteMemory8(const simAddress sa, const uint8 Data, 
			  const bool spec)
{
  if (spec) {
    return specMem.writeSpec8(sa, Data);
  } else {
    uint8 *Page = GetPage(sa);
    if(!Page)     return(0);
    
    Page[sa&PageMask] = Data;
    return ( true );
  }
} 

//:Write 2 bytes
bool base_memory::_WriteMemory16(const simAddress sa, const uint16 Data,
			   const bool spec)
{
  if (spec) {
    return specMem.writeSpec16(sa, Data);
  } else {
    uint8 *Page = GetPage(sa);
    if(!Page)     return(0);

    unsigned int index = sa&PageMask;
    if (index + 2 > PageSize) {
	uint8 *Page2 = GetPage(sa+2);
	memcpy(&(Page[index]), &Data, 1);
	memcpy(Page2, ((uint8*)&Data)+1, 1);
    } else {
	Page = &Page[index];
	uint16        *Temp = (uint16*)Page;
        memcpy(Temp, &Data, sizeof(uint16));
    }
    return ( true );
  }
}

//:Write 4 bytes
bool base_memory::_WriteMemory32(const simAddress sa, const uint32 Data,
			   const bool spec)
{
  if (spec) {
    return specMem.writeSpec32(sa, Data);
  } else {
    uint8 *Page = GetPage(sa);
    if(!Page)     return(0);

    unsigned int index = sa&PageMask;
    if (index + 4 > PageSize) {
	uint8 *Page2 = GetPage(sa+4);
	memcpy(&(Page[index]), &Data, PageSize-index);
	memcpy(Page2, ((uint8*)&Data)+PageSize-index, 4-(PageSize-index));
    } else {
	Page = &Page[index];
	uint32 *Temp = (uint32*)Page;
        memcpy(Temp, &Data, sizeof(uint32));
    }
    return ( true );
  }
}

//:Write 8 bytes
bool base_memory::_WriteMemory64(const simAddress sa, const uint64 Data,
			   const bool spec)
{
  if (spec) {    
    printf("fix me %s %d\n", __FILE__, __LINE__);
    return specMem.writeSpec32(sa, Data);
  } else {
    uint8 *Page = GetPage(sa);
    if(!Page)     return(0);

    unsigned int index = sa&PageMask;
    if (index + 8 > PageSize) {
      printf("split write %s %d\n", __FILE__, __LINE__);
	uint8 *Page2 = GetPage(sa+8);
	memcpy(&(Page[index]), &Data, PageSize-index);
	memcpy(Page2, ((uint8*)&Data)+PageSize-index, 8-(PageSize-index));
    } else {
	Page = &Page[index];
	uint64 *Temp = (uint64*)Page;
        memcpy(Temp, &Data, sizeof(uint64));
    }
    return ( true );
  }
}

bool base_memory::LoadMem( const simAddress dest, void* const source, int Bytes)
{ 
  DPRINT(0,"dest=%#x source=%p Bytes=%d\n",dest,source,Bytes);
  uint8_t* srcptr = (unsigned char *)source;
  simAddress dest_cursor = dest;

  for( ; Bytes > 0; Bytes--) {
    if ( ! _WriteMemory8(dest_cursor, (uint8_t)*srcptr, 0) ) {
      ERROR( "dest=0x%x source=%p Bytes=%i\n", dest_cursor,srcptr,Bytes);
      return false;
    }
    srcptr++;
    dest_cursor ++;
  }
  return true;
}

//: Register a memory mapping (in other words, set up a virtual memory mapping)
/*int base_memory::RegisterMem(memory_interface *mem, simAddress addr,
        int len, simAddress farOffset, int writeLat, component *comp)
{
    MemMapEntry entry = {mem,addr,len,farOffset,writeLat, comp};

    DPRINT(0,"addr=%#x len=%#x farOffset=%#x writeLat=%d\n",
                addr,len,farOffset,writeLat);

    memMapByAddr.insert(MemMapByAddr::value_type( addr,entry) );

    return 0;
}

void base_memory::RegisterMemIF( string cfgStr, memory_interface *memIF, 
						component *notify )
{
  int addr,len,farOffset,writeLat;
  addr = configuration::getValue( cfgStr + ":addr" );
  if ( _config_error ) {
    ERROR("%s:addr not defined\n",cfgStr.c_str());
  }

  len = configuration::getValue( cfgStr + ":len" );
  if ( _config_error ) {
    ERROR("%s:len not defined\n",cfgStr.c_str());
  }

  farOffset = configuration::getValue( cfgStr + ":farOffset" );
  if ( _config_error ) {
    ERROR("%s:farOffset not defined\n",cfgStr.c_str());
  }

  writeLat = configuration::getValue( cfgStr + ":writeLat" );
  if ( _config_error ) {
    ERROR("%s:writeLat not defined\n",cfgStr.c_str());
  }

  DPRINT(0,"cfg=%s memIF=%p addr=%#x len=%#x farOffset=%#x\n",
                        cfgStr.c_str(), memIF, addr, len, farOffset );
  RegisterMem( memIF, addr, len, farOffset, writeLat, notify );
}

MemMapEntry* base_memory::FindMemByAddr( simAddress addr)
{
    if ( memMapByAddr.empty() ) {
        return NULL;
    }

    MemMapByAddr::iterator iter = memMapByAddr.upper_bound(addr);
    if (iter == memMapByAddr.begin() ) {
      return NULL;
    }
    --iter;

    MemMapEntry entry = (*iter).second;

    if ( addr >= entry.addr &&
                        (uint64) addr < (uint64) entry.addr + entry.len) {
        DPRINT(1,"found device for addr=%#x\n",addr);
        return &(*iter).second;
    }
    return NULL;
    }

void base_memory::ReadNotify( component *comp, simAddress addr, int size )
{
    parcel *p = parcel::newParcel();
    p->data() = (void*) addr;
    p->tag() = ( size << 16 ) | ReadOp;
    sendParcel( p, comp, TimeStamp() );
}
void base_memory::WriteNotify( component *comp, simAddress addr, int size )
{
    parcel *p = parcel::newParcel();
    p->data() = (void*) addr;
    p->tag() = ( size << 16 ) | WriteOp;
    sendParcel( p, comp, TimeStamp() );
    }*/

uint8 memory::ReadMemory8(const simAddress sa, const bool s) {
    return myMem->ReadMemory8(sa, s);
}
bool memory::WriteMemory8(const simAddress sa, const uint8 d, const bool s) {
    return myMem->WriteMemory8(sa, d, s);
}
uint16 memory::ReadMemory16(const simAddress sa, const bool s) {
    return myMem->ReadMemory16(sa, s);
}
bool memory::WriteMemory16(const simAddress sa, const uint16 d, const bool s) {
    return myMem->WriteMemory16(sa, d, s);
}
uint32 memory::ReadMemory32(const simAddress sa, const bool s) {
    return myMem->ReadMemory32(sa, s);
}
bool memory::WriteMemory32(const simAddress sa, const uint32 d, const bool s) {
    return myMem->WriteMemory32(sa, d, s);
}
uint64 memory::ReadMemory64(const simAddress sa, const bool s) {
    return myMem->ReadMemory64(sa, s);
}
bool memory::WriteMemory64(const simAddress sa, const uint64 d, const bool s) {
    return myMem->WriteMemory64(sa, d, s);
}

