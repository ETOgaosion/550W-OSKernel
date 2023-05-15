#ifndef MM_H
#define MM_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common/type.h>

#define FS_KERNEL_ADDR 0xffffffc054000000lu

#define START_BLOCK 2000
#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
#define INIT_KERNEL_STACK 0xffffffc051000000lu
#define FREEMEM (INIT_KERNEL_STACK + PAGE_SIZE*10)
#define USER_STACK_ADDR 0xf00010000lu

/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

typedef uint64_t PTE;
extern ptr_t memCurr;
extern int diskpg_num;
extern uint64_t diskpg[1000];
extern void movetodisk(uint64_t pg_kva, uint64_t user_va);
extern void getbackdisk(uint64_t va, uint64_t new_addr);
extern void en_invalid(uint64_t pa_kva, PTE* pgdir);
extern ptr_t allocmem(int numPage, uint64_t user_va);
extern ptr_t allocPage(int numPage);
extern void fork_pgtable(PTE* dest_pgdir, PTE* src_pgdir);
extern void fork_page_helper(uintptr_t va, PTE* destpgdir, PTE* srcpgdir);
extern uint64_t get_kva_from_va(uint64_t va, PTE* pgdir);
extern uint64_t do_getpa(uint64_t va);
extern void map_kp(uint64_t va, uint64_t pa, PTE *pgdir);

//extern ptr_t allocPage(int numPage);
extern void freePage(ptr_t baseAddr, int numPage);
extern void* kmalloc(size_t size);
extern void share_pgtable(PTE* dest_pgdir, PTE* src_pgdir);
extern uintptr_t alloc_page_helper(uintptr_t va, PTE* pgdir);
uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);
extern void map(uint64_t va, uint64_t pa, PTE *pgdir);
extern void getback_page(int pid);
extern uint64_t get_kva(PTE entry);

#endif /* MM_H */
