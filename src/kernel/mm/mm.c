#include <asm/pgtable.h>
#include <asm/sbi.h>
#include <common/elf.h>
#include <drivers/screen/screen.h>
#include <lib/assert.h>
#include <lib/math.h>
#include <os/mm.h>
#include <os/pcb.h>
#include <os/smp.h>

#define MAXPAGES 1000
#define STARTPAGE 30

ptr_t memCurr = FREEMEM;
ptr_t k_mem = FREEMEMK;

int diskpg_num = 0;
int freepg_num = 0;

uint64_t diskpg[1000];
uint64_t freepg[1000];
uint64_t allpg[20][MAXPAGES];
uint64_t allmem[20][MAXPAGES];
uint64_t diskpg[1000];
uint64_t alluserva[20][100];
int shm_num = 0;
struct shm {
    int pro_num;
    int key;
    uint64_t kva;
    uint64_t uva;
} shm_all[10];

PTE *k_mm_get_kva(PTE entry) {
    uint64_t pa_temp = (entry >> _PAGE_PFN_SHIFT) % ((uint64_t)1 << SATP_ASID_SHIFT);
    pa_temp = pa_temp << 12;
    return pa2kva(pa_temp);
}

void k_mm_getback_page(int pid) {
    if (freepg_num >= 999) {
        return;
    }
    for (int i = 1; i <= allpg[pid][0]; i++) {
        freepg_num++;
        freepg[freepg_num] = allpg[pid][i];
    }
    for (int i = 1; i <= allmem[pid][0]; i++) {
        freepg_num++;
        freepg[freepg_num] = allmem[pid][i];
        for (int j = 0; j < shm_num; j++) {
            if (shm_all[j].kva == allmem[pid][i]) {
                freepg_num--;
            }
        }
    }
    // freepg[pid][allpg[pid][0]+1-i] = allpg[pid][i];
    allpg[pid][0] = 0;
    allmem[pid][0] = 0;
}

// start block = 250
void k_mm_move_to_disk(uint64_t pg_kva, uint64_t user_va) {
    // one block = 512B = 0.5KB
    if (diskpg_num >= 999) {
        assert(0);
    }
    sys_screen_move_cursor(1, 5);
    prints("[k] From 0x%x to disk sector %d, for user_vaddr 0x%x\n", pg_kva, START_BLOCK + diskpg_num * 2, user_va);

    // sbi_sd_write(kva2pa(pg_kva), 2, START_BLOCK + diskpg_num * 2);
    diskpg[diskpg_num] = user_va;
    // diskpg[diskpg_num][1] = diskpg_num;
    diskpg_num++;
    clear_pgdir(pg_kva);
}

void k_mm_get_back_disk(uint64_t user_va, uint64_t new_addr) {
    int flag = 0;
    for (int i = 0; i < diskpg_num; i++) {
        if (diskpg[i] == user_va) {
            sys_screen_move_cursor(1, 6);
            prints("[k] From disk sector %d to 0x%x, for user_vaddr 0x%x\n", START_BLOCK + i * 2, new_addr, user_va);

            flag = 1;
            // sbi_sd_read(kva2pa(new_addr), 2, START_BLOCK + i * 2);
            break;
        }
    }
    if (flag == 0) {
        assert(0);
    }
}

void k_mm_en_invalid(uint64_t va, PTE *pgdir) {
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));
    if (pgdir[vpn2] == 0) {
        assert(0);
    }
    PTE *pmd = k_mm_get_kva(pgdir[vpn2]);
    if (pmd[vpn1] == 0) {
        assert(0);
    }
    PTE *pld = k_mm_get_kva(pmd[vpn1]);
    pld[vpn0] = (pld[vpn0] >> 1) << 1;
}

ptr_t k_mm_alloc_mem(int numPage, uint64_t user_va) {
    ptr_t ret;
    int allocpid = (*current_running)->pid;
    if (allmem[allocpid][0] >= MAXPAGES) {
        // no enough page allowed
        ret = allmem[allocpid][STARTPAGE];
        k_mm_move_to_disk(allmem[allocpid][STARTPAGE], alluserva[allocpid][STARTPAGE]);
        k_mm_en_invalid(alluserva[allocpid][STARTPAGE], (pa2kva(pcb[allocpid].pgdir << 12)));
        for (int i = STARTPAGE; i < allmem[allocpid][0]; i++) {
            allmem[allocpid][i] = allmem[allocpid][i + 1];
        }
        for (int i = STARTPAGE; i < alluserva[allocpid][0]; i++) {
            alluserva[allocpid][i] = alluserva[allocpid][i + 1];
        }
        allmem[allocpid][allmem[allocpid][0]] = ret;
        alluserva[allocpid][alluserva[allocpid][0]] = user_va;
        return ret;
    }
    if (freepg_num == 0) {
        ret = K_ROUND(memCurr, PAGE_SIZE);
        memCurr = ret + numPage * PAGE_SIZE;
    } else {
        ret = freepg[freepg_num];
        freepg_num--;
    }
    if (allocpid >= 0) {
        if (allmem[allocpid][0] >= MAXPAGES) {
            return ret;
        }
        allmem[allocpid][0]++;
        allmem[allocpid][allmem[allocpid][0]] = ret;
        alluserva[allocpid][0]++;
        alluserva[allocpid][alluserva[allocpid][0]] = user_va;
    }
    clear_pgdir(ret);
    return ret;
}
// return kva
ptr_t k_mm_alloc_page(int numPage) {
    ptr_t ret;
    if (freepg_num == 0) {
        ret = K_ROUND(memCurr, PAGE_SIZE);
        memCurr = ret + numPage * PAGE_SIZE;
    } else {
        ret = freepg[freepg_num];
        freepg_num--;
    }
    int allocpid = (*current_running)->pid;
    if (allocpid >= 0) {
        if (allpg[allocpid][0] >= 98) {
            return ret;
        }
        allpg[allocpid][0]++;
        allpg[allocpid][allpg[allocpid][0]] = ret;
    }
    clear_pgdir(ret);
    return ret;
}

void k_mm_free_page(ptr_t baseAddr, int numPage) {}

void *k_mm_malloc(size_t size) {
    ptr_t ret = K_ROUND(k_mem, 4);
    k_mem = ret + size;
    return (void *)ret;
}

uint64_t k_mm_alloc_newva() {
    int allocpid = (*current_running)->pid;
    for (uint64_t i = 0x100000; i < 0x1000000; i += 0x1000) {
        int flag = 1;
        for (int j = 1; j <= alluserva[allocpid][0]; j++) {
            if (i == alluserva[allocpid][j]) {
                flag = 0;
            }
        }
        if (flag) {
            return i;
        }
    }
    return 0;
}

long k_mm_shm_page_get(int key) {
    for (int i = 0; i < shm_num; i++) {
        if (shm_all[i].key == key && shm_all[i].pro_num != 0) {
            k_mm_map(shm_all[i].uva, kva2pa(shm_all[i].kva), (pa2kva((*current_running)->pgdir << 12)));
            shm_all[i].pro_num++;
            return shm_all[i].uva;
        }
    }
    int num = -1;
    for (int i = 0; i < shm_num; i++) {
        if (shm_all[i].pro_num == 0) {
            num = i;
            break;
        }
    }
    if (num == -1) {
        num = shm_num;
    }
    shm_all[num].key = key;
    shm_all[num].uva = k_mm_alloc_newva();
    shm_all[num].kva = k_mm_alloc_page_helper(shm_all[num].uva, (pa2kva((*current_running)->pgdir << 12)));
    shm_all[num].pro_num = 1;
    if (num == shm_num) {
        shm_num++;
    }
    return shm_all[num].uva;
}

long k_mm_shm_page_dt(uintptr_t addr) {
    for (int i = 0; i < shm_num; i++) {
        if (shm_all[i].uva == addr) {
            uint64_t va = addr;
            PTE *pgdir = pa2kva((*current_running)->pgdir << 12);
            va &= VA_MASK;
            uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
            uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
            uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));
            if (pgdir[vpn2] == 0) {
                assert(0);
            }
            PTE *pmd = k_mm_get_kva(pgdir[vpn2]);
            if (pmd[vpn1] == 0) {
                assert(0);
            }
            PTE *pld = k_mm_get_kva(pmd[vpn1]);
            pld[vpn0] = 0;
            // cancel finished
            shm_all[i].pro_num--;
            if (shm_all[i].pro_num == 0) {
                freepg_num++;
                freepg[freepg_num] = shm_all[i].kva;
                // this uva has been sended out
                // shm_all[i].pro_num = 1;
            }
        }
    }
    return 0;
}

/* this is used for mapping kernel virtual address into user page table */
void k_mm_share_pgtable(PTE *dest_pgdir, PTE *src_pgdir) {
    for (uint64_t vpn2 = 0; vpn2 < 512; vpn2++) {
        if (src_pgdir[vpn2] % 2 == 1) {
            dest_pgdir[vpn2] = 0;
            uint64_t pfn = get_pfn(src_pgdir[vpn2]);
            set_pfn(&dest_pgdir[vpn2], pfn);
            set_attribute(&dest_pgdir[vpn2], _PAGE_PRESENT);
        }
    }
}

void k_mm_fork_pgtable(PTE *dest_pgdir, PTE *src_pgdir) {
    uint64_t sva = USER_STACK_ADDR - PAGE_SIZE;
    sva &= VA_MASK;
    uint64_t va = 0;
    // kernel not to clear write bit
    for (uint64_t vpn2 = 0; vpn2 < 256; vpn2++) {
        if (src_pgdir[vpn2] % 2 == 1) {
            PTE *pmd = k_mm_get_kva(src_pgdir[vpn2]);
            for (uint64_t vpn1 = 0; vpn1 < 512; vpn1++) {
                if (pmd[vpn1] % 2 == 1) {
                    PTE *pld = k_mm_get_kva(pmd[vpn1]);
                    for (uint64_t vpn0 = 0; vpn0 < 512; vpn0++) {
                        if ((pld[vpn0] % 2) == 1) {
                            va = (vpn2 << (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS)) | (vpn1 << (NORMAL_PAGE_SHIFT + PPN_BITS)) | (vpn0 << NORMAL_PAGE_SHIFT);
                            if (va >= (uint64_t)KERNEL_START_PA && va <= (uint64_t)KERNEL_END_PA) {
                                continue;
                            }
                            // no stack, clear write bit
                            if (va != sva && ((va & ((uint64_t)1 << 39)) == 0)) {
                                uint64_t pa_kva = (uint64_t)k_mm_get_kva(pld[vpn0]);
                                // src pgdir
                                pld[vpn0] = ((pld[vpn0] >> 3) << 3) | 3;
                                // k_print("vpn: %d, %d, %d, va:0x%x, pa:0x%x\n",
                                // vpn2,vpn1,vpn0, va<<NORMAL_PAGE_SHIFT,
                                // kva2pa(pa_kva));

                                k_mm_map(va, kva2pa(pa_kva), dest_pgdir);
                                // dest pgdir
                                PTE *spmd = k_mm_get_kva(dest_pgdir[vpn2]);
                                PTE *spld = k_mm_get_kva(spmd[vpn1]);
                                spld[vpn0] = ((spld[vpn0] >> 3) << 3) | 3;
                            }
                        }
                    }
                }
            }
            // uint64_t pfn = get_pfn(src_pgdir[vpn2]);
            // set_pfn(&dest_pgdir[vpn2], pfn);
            // set_attribute(&dest_pgdir[vpn2], _PAGE_PRESENT);
        }
    }
}

void k_mm_fork_page_helper(uintptr_t va, PTE *destpgdir, PTE *srcpgdir) {
    uint64_t dest_kva = k_mm_alloc_page_helper(va, destpgdir);
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));
    if (srcpgdir[vpn2] == 0) {
        assert(0);
    }
    PTE *pmd = k_mm_get_kva(srcpgdir[vpn2]);
    if (pmd[vpn1] == 0) {
        assert(0);
    }
    PTE *pld = k_mm_get_kva(pmd[vpn1]);
    uint64_t src_kva = (uint64_t)k_mm_get_kva(pld[vpn0]);

    unsigned char *src = (unsigned char *)src_kva;
    unsigned char *dest = (unsigned char *)dest_kva;
    for (int i = 0; i < 0x1000; i++) {
        dest[i] = src[i];
    }
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
uintptr_t k_mm_alloc_page_helper(uintptr_t va, PTE *pgdir) {
    uint64_t kva = k_mm_alloc_mem(1, va);
    uint64_t pa = kva2pa(kva);
    k_mm_map(va, pa, pgdir);
    return kva;
}

uint64_t k_mm_get_kva_from_va(uint64_t va, PTE *pgdir) {
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));
    if (pgdir[vpn2] == 0) {
        assert(0);
    }
    PTE *pmd = k_mm_get_kva(pgdir[vpn2]);
    if (pmd[vpn1] == 0) {
        assert(0);
    }
    PTE *pld = k_mm_get_kva(pmd[vpn1]);
    return (uint64_t)k_mm_get_kva(pld[vpn0]);
}

long k_mm_getpa(uint64_t va) {
    // screen_move_cursor(1,6);
    // prints("in mmc pre: %lx\n", (uint64_t)(va));
    uint64_t kva = k_mm_get_kva_from_va(va, pa2kva((*current_running)->pgdir << 12));
    // screen_move_cursor(1,7);
    // prints("in mmc: %lx\n", (uint64_t)(kva));
    kva += va % 0x1000;
    return kva2pa(kva);
}

void k_mm_map_kp(uint64_t va, uint64_t pa, PTE *pgdir) {
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));
    if (pgdir[vpn2] == 0) {
        // alloc a new second-level page directory
        set_pfn(&pgdir[vpn2], kva2pa(k_mm_alloc_page(1)) >> NORMAL_PAGE_SHIFT);
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        clear_pgdir((uintptr_t)k_mm_get_kva(pgdir[vpn2]));
    }
    PTE *pmd = k_mm_get_kva(pgdir[vpn2]);
    if (pmd[vpn1] == 0) {
        set_pfn(&pmd[vpn1], kva2pa(k_mm_alloc_page(1)) >> NORMAL_PAGE_SHIFT);
        set_attribute(&pmd[vpn1], _PAGE_PRESENT);
        clear_pgdir((uintptr_t)k_mm_get_kva(pmd[vpn1]));
    }
    PTE *pld = k_mm_get_kva(pmd[vpn1]);
    pld[vpn0] = 0;
    set_pfn(&pld[vpn0], pa >> NORMAL_PAGE_SHIFT);
    /*set_attribute(
        &pld[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                  _PAGE_USER| _PAGE_EXEC);
    */
    set_attribute(&pld[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
}

void map_fs_space() {
    int size = 1000;
    uint64_t offset = 0;
    for (int i = 0; i < size; i++) {
        k_mm_map_kp(FS_KERNEL_ADDR + offset, kva2pa(FS_KERNEL_ADDR), pa2kva(PGDIR_PA));
        offset += 0x1000;
    }
}

void k_mm_map(uint64_t va, uint64_t pa, PTE *pgdir) {
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((va >> (NORMAL_PAGE_SHIFT + PPN_BITS)) << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));
    if (pgdir[vpn2] == 0) {
        // alloc a new second-level page directory
        set_pfn(&pgdir[vpn2], kva2pa(k_mm_alloc_page(1)) >> NORMAL_PAGE_SHIFT);
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        clear_pgdir((uintptr_t)k_mm_get_kva(pgdir[vpn2]));
    }
    PTE *pmd = k_mm_get_kva(pgdir[vpn2]);
    if (pmd[vpn1] == 0) {
        set_pfn(&pmd[vpn1], kva2pa(k_mm_alloc_page(1)) >> NORMAL_PAGE_SHIFT);
        set_attribute(&pmd[vpn1], _PAGE_PRESENT);
        clear_pgdir((uintptr_t)k_mm_get_kva(pmd[vpn1]));
    }
    PTE *pld = k_mm_get_kva(pmd[vpn1]);
    pld[vpn0] = 0;
    set_pfn(&pld[vpn0], pa >> NORMAL_PAGE_SHIFT);
    set_attribute(&pld[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_USER | _PAGE_EXEC);
    /*
    set_attribute(
        &pld[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                  _PAGE_USER| _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
    */
}

void k_mm_cancel_pg(PTE *pgdir) {
    for (uint64_t kva = KERNEL_START_PA; kva < 0x80400000lu; kva += 0x200000lu) {
        uint64_t va = kva & VA_MASK;
        uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
        pgdir[vpn2] = 0;
    }
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */

uint32_t check_page_map(uintptr_t vta, uintptr_t pgdir) {
    uint64_t vpn[] = {
        (vta >> 12) & ~(~0 << 9), // vpn0
        (vta >> 21) & ~(~0 << 9), // vpn1
        (vta >> 30) & ~(~0 << 9)  // vpn2
    };
    PTE *page_base = (uint64_t *)pgdir;
    PTE *second_page = NULL;
    PTE *third_page = NULL;
    if (((page_base[vpn[2]]) & _PAGE_PRESENT) == 0) {
        return 1;
    } else {
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }

    if (((second_page[vpn[1]]) & _PAGE_PRESENT) == 0) {
        return 1;
    } else {
        third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    }

    if ((third_page[vpn[0]] & _PAGE_PRESENT) == 0) {
        return 1;
    } else {
        return 0;
    }
}

uintptr_t free_page_helper(uintptr_t va, uintptr_t pgdir) {
    uint64_t vpn[] = {(va >> 12) & ~(~0 << 9), (va >> 21) & ~(~0 << 9), (va >> 30) & ~(~0 << 9)};
    PTE *page_base = (uint64_t *)pgdir;
    PTE *second_page = NULL;
    PTE *third_page = NULL;
    if ((page_base[vpn[2]] & _PAGE_PRESENT) == 0) {
        return -1;
    }
    second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    if ((second_page[vpn[1]] & _PAGE_PRESENT) == 0) {
        return -1;
    }
    third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    k_mm_free_page(get_kva_of(va, pgdir), 1);
    third_page[vpn[0]] = 0;
    return 0;
}

// [TODO]
long sys_shmget(key_t key, size_t size, int flag) {
    return 0;
}

long sys_shmctl(int shmid, int cmd, shmid_ds_t *buf) {
    return 0;
}

long sys_shmat(int shmid, char *shmaddr, int shmflg) {
    return 0;
}

long sys_shmdt(char *shmaddr) {
    return 0;
}

long sys_brk(unsigned long brk) {
    if (brk > K_ROUNDDOWN((*current_running)->user_sp, NORMAL_PAGE_SIZE) - STACK_SIZE) {
        return -EINVAL;
    }
    if (brk == 0) {
        return (*current_running)->elf.edata;
    }
    if (brk < (*current_running)->elf.edata) {
        for (uintptr_t iterator = brk; iterator < (*current_running)->elf.edata; iterator += NORMAL_PAGE_SIZE) {
            if (get_kva_of(iterator, (*current_running)->pgdir)) {
                free_page_helper(iterator, (*current_running)->pgdir);
            }
        }
    } else {
        for (uintptr_t iterator = (*current_running)->elf.edata; iterator < brk; iterator += NORMAL_PAGE_SIZE) {
            k_mm_alloc_page_helper(iterator, (pa2kva((*current_running)->pgdir)));
            local_flush_tlb_all();
        }
    }
    (*current_running)->elf.edata = brk;
    return brk;
}

long sys_swapon(const char *specialfile, int swap_flags) {
    return 0;
}

long sys_swapoff(const char *specialfile) {
    return 0;
}

long sys_mprotect(unsigned long start, size_t len, unsigned long prot) {
    return 0;
}

long sys_msync(unsigned long start, size_t len, int flags) {
    return 0;
}

long sys_mlock(unsigned long start, size_t len) {
    return 0;
}

long sys_munlock(unsigned long start, size_t len) {
    return 0;
}

long sys_madvise(unsigned long start, size_t len, int behavior) {
    return 0;
}