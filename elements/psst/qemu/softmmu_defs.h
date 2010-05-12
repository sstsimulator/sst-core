#ifndef SOFTMMU_DEFS_H
#define SOFTMMU_DEFS_H

uint8_t REGPARM __ldb_mmu(target_ulong addr, int mmu_idx);
void REGPARM __stb_mmu(target_ulong addr, uint8_t val, int mmu_idx);
uint16_t REGPARM __ldw_mmu(target_ulong addr, int mmu_idx);
void REGPARM __stw_mmu(target_ulong addr, uint16_t val, int mmu_idx);
uint32_t REGPARM __ldl_mmu(target_ulong addr, int mmu_idx);
void REGPARM __stl_mmu(target_ulong addr, uint32_t val, int mmu_idx);

static uint8_t REGPARM __ldb_mmu_ra(target_ulong addr, int mmu_idx, void* ra);
static uint32_t REGPARM __ldl_mmu_ra(target_ulong, int, void*);
static uint64_t REGPARM __ldq_mmu_ra(target_ulong, int, void*);

static void slow_stb_mmu(target_ulong, uint8_t, int, void*);
//static uint8_t slow_ldb_mmu(target_ulong, int, void*);
static void slow_stw_mmu(target_ulong, uint16_t, int, void*);
static void slow_stl_mmu(target_ulong, uint32_t, int, void*);
static void slow_stq_mmu(target_ulong, uint64_t, int, void*);
//static uint32_t slow_ldl_mmu(target_ulong, int, void*);
//static uint64_t slow_ldq_mmu(target_ulong, int, void*);

uint64_t REGPARM __ldq_mmu(target_ulong addr, int mmu_idx);
void REGPARM __stq_mmu(target_ulong addr, uint64_t val, int mmu_idx);

uint8_t REGPARM __ldb_cmmu(target_ulong addr, int mmu_idx);
void REGPARM __stb_cmmu(target_ulong addr, uint8_t val, int mmu_idx);
uint16_t REGPARM __ldw_cmmu(target_ulong addr, int mmu_idx);
void REGPARM __stw_cmmu(target_ulong addr, uint16_t val, int mmu_idx);
uint32_t REGPARM __ldl_cmmu(target_ulong addr, int mmu_idx);
void REGPARM __stl_cmmu(target_ulong addr, uint32_t val, int mmu_idx);
uint64_t REGPARM __ldq_cmmu(target_ulong addr, int mmu_idx);
void REGPARM __stq_cmmu(target_ulong addr, uint64_t val, int mmu_idx);

#endif
