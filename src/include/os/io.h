#pragma once

// using this as IO address space (at most using 1 GB, so that it can be store
// in one pgdir entry)
#define IO_ADDR_START 0xffffffe000000000lu

void *k_ioremap(unsigned long phys_addr, unsigned long size);
void k_iounmap(void *io_addr);
