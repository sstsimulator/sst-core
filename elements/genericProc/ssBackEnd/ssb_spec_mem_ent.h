#ifndef SSB_SPEC_MEM_ENT_H
#define SSB_SPEC_MEM_ENT_H


/* speculative memory hash table size, NOTE: this must be a power-of-two */
#define STORE_HASH_SIZE		32

//: Speculative memory hash table 
//
// speculative memory hash table definition, accesses go through this
// hash table when accessing memory in speculative mode, the hash
// table flush the table when recovering from mispredicted branches
//
//!SEC:ssBack
struct spec_mem_ent {
  struct spec_mem_ent *next;		/* ptr to next hash table bucket */
  md_addr_t addr;			/* virtual address of spec state */
  unsigned int data[2];			/* spec buffer, up to 8 bytes */
};


#endif
