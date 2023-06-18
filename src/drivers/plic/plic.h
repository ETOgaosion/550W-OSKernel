#pragma once

#define UART_IRQ 10
#define DISK_IRQ 1

void d_plic_init(void);

// enable PLIC for each hart
void d_plic_init_hart(void);

// ask PLIC what interrupt we should serve
int d_plic_claim(void);

// tell PLIC that we've served this IRQ
void d_plic_complete(int irq);