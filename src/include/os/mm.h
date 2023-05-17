#pragma once

#include <common/types.h>

#define FS_KERNEL_ADDR 0xffffffc054000000lu

#define START_BLOCK 2000
#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
#define INIT_KERNEL_STACK 0xffffffc051000000lu
#define FREEMEM (INIT_KERNEL_STACK + PAGE_SIZE * 10)
#define USER_STACK_ADDR 0xf00010000lu

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define PROT_GROWSDOWN 0X01000000
#define PROT_GROWSUP 0X02000000

#define MAP_FILE 0
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0X02
#define MAP_FAILED ((void *) -1)

/* Rounding; only works for n = power of two */
#define ROUND(a, n) (((((uint64_t)(a)) + (n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

typedef uint64_t PTE;
extern ptr_t memCurr;
extern int diskpg_num;
extern uint64_t diskpg[1000];

void movetodisk(uint64_t pg_kva, uint64_t user_va);
void getbackdisk(uint64_t va, uint64_t new_addr);
void en_invalid(uint64_t pa_kva, PTE *pgdir);
ptr_t allocmem(int numPage, uint64_t user_va);
ptr_t allocPage(int numPage);
void fork_pgtable(PTE *dest_pgdir, PTE *src_pgdir);
void fork_page_helper(uintptr_t va, PTE *destpgdir, PTE *srcpgdir);
uint64_t get_kva_from_va(uint64_t va, PTE *pgdir);
long sys_getpa(uint64_t va);
void map_kp(uint64_t va, uint64_t pa, PTE *pgdir);

// ptr_t allocPage(int numPage);
void freePage(ptr_t baseAddr, int numPage);
void *kmalloc(size_t size);
void share_pgtable(PTE *dest_pgdir, PTE *src_pgdir);
uintptr_t alloc_page_helper(uintptr_t va, PTE *pgdir);
long sys_shm_page_get(int key);
long sys_shm_page_dt(uintptr_t addr);
void map(uint64_t va, uint64_t pa, PTE *pgdir);
void getback_page(int pid);
PTE *get_kva(PTE entry);
void cancelpg(PTE *pgdir);
