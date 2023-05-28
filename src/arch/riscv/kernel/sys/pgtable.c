#include <asm/pgtable.h>

void clear_pgdir(uintptr_t pgdir_addr) {
    PTE *pgaddr = (PTE *)pgdir_addr;
    for (int i = 0; i < 512; i++) {
        pgaddr[i] = 0;
    }
}