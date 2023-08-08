#include <asm/pgtable.h>
#include <common/types.h>
#include <os/io.h>
#include <os/mm.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;
int io_count = 0;

void *k_ioremap(unsigned long phys_addr, unsigned long size) {
    // map phys_addr to a virtual address
    // then return the virtual address
    int isize = size;
    uint64_t va = io_base + 0x1000 * io_count;
    while (isize) {
        k_mm_map_kp(io_base + 0x1000 * io_count, phys_addr, pa2kva(PGDIR_PA));
        io_count++;
        isize -= 0x1000;
        phys_addr += 0x1000;
    }
    // set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
    return (void *)va;
}

void k_iounmap(void *io_addr) {
    // maybe no one would call this function?
    // *pte = 0;
}
