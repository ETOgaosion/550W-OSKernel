#pragma once

#define UART_IRQ 10
#define DISK_IRQ 1

void plic_init(void);

// enable PLIC for each hart
void plic_init_hart(void);

// ask PLIC what interrupt we should serve
int plic_claim(void);

// tell PLIC that we've served this IRQ
void plic_complete(int irq);