#pragma once

void setup_vm();

void enable_vm();

int prepare_vm(unsigned long mhartid, uintptr_t riscv_dtb);