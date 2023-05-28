#pragma once

void plic_init(void);
void plic_init_hart(void);
int plic_claim(void);
void plic_complete(int);