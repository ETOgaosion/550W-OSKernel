#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;
int io_count = 0;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    int isize = size;
    uint64_t va = io_base+0x1000*io_count;
    while(isize)
    {
        map_kp(io_base+0x1000*io_count, phys_addr, pa2kva(PGDIR_PA));
        io_count++;
        isize-=0x1000;
        phys_addr+=0x1000;
    }
    //set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
    return va;
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}
