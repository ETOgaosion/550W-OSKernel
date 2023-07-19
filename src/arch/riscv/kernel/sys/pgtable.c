#include <asm/pgtable.h>

void clear_pgdir(uintptr_t pgdir_addr) {
    PTE *pgaddr = (PTE *)pgdir_addr;
    for (int i = 0; i < 512; i++) {
        pgaddr[i] = 0;
    }
}

void set_pfn(PTE *entry, uint64_t pfn) {
    uint64_t temp10 = (*entry) % ((uint64_t)1 << 10);
    *entry = (pfn << 10) | temp10;
}

void set_attribute(PTE *entry, uint64_t bits) {
    uint64_t pfn_temp = (*entry) >> 10;
    *entry = (pfn_temp << 10) | bits;
}