#pragma once

#include <common/types.h>

// which hart (core) is this?
static inline uint64_t asm_r_mhartid() {
    uint64_t x;
    asm volatile("csrr %0, mhartid" : "=r"(x));
    return x;
}

// Machine Status Register, mstatus

#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3) // machine-mode interrupt enable.

static inline uint64_t asm_r_mstatus() {
    uint64_t x;
    asm volatile("csrr %0, mstatus" : "=r"(x));
    return x;
}

static inline void asm_w_mstatus(uint64_t x) {
    asm volatile("csrw mstatus, %0" : : "r"(x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void asm_w_mepc(uint64_t x) {
    asm volatile("csrw mepc, %0" : : "r"(x));
}

// Supervisor Status Register, sstatus

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

static inline uint64_t asm_r_sstatus() {
    uint64_t x;
    asm volatile("csrr %0, sstatus" : "=r"(x));
    return x;
}

static inline void asm_w_sstatus(uint64_t x) {
    asm volatile("csrw sstatus, %0" : : "r"(x));
}

// Supervisor Interrupt Pending
static inline uint64_t asm_r_sip() {
    uint64_t x;
    asm volatile("csrr %0, sip" : "=r"(x));
    return x;
}

static inline void asm_w_sip(uint64_t x) {
    asm volatile("csrw sip, %0" : : "r"(x));
}

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software
static inline uint64_t asm_r_sie() {
    uint64_t x;
    asm volatile("csrr %0, sie" : "=r"(x));
    return x;
}

static inline void asm_w_sie(uint64_t x) {
    asm volatile("csrw sie, %0" : : "r"(x));
}

// Machine-mode Interrupt Enable
#define MIE_MEIE (1L << 11) // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software
static inline uint64_t asm_r_mie() {
    uint64_t x;
    asm volatile("csrr %0, mie" : "=r"(x));
    return x;
}

static inline void asm_w_mie(uint64_t x) {
    asm volatile("csrw mie, %0" : : "r"(x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void asm_w_sepc(uint64_t x) {
    asm volatile("csrw sepc, %0" : : "r"(x));
}

static inline uint64_t asm_r_sepc() {
    uint64_t x;
    asm volatile("csrr %0, sepc" : "=r"(x));
    return x;
}

// Machine Exception Delegation
static inline uint64_t asm_r_medeleg() {
    uint64_t x;
    asm volatile("csrr %0, medeleg" : "=r"(x));
    return x;
}

static inline void asm_w_medeleg(uint64_t x) {
    asm volatile("csrw medeleg, %0" : : "r"(x));
}

// Machine Interrupt Delegation
static inline uint64_t asm_r_mideleg() {
    uint64_t x;
    asm volatile("csrr %0, mideleg" : "=r"(x));
    return x;
}

static inline void asm_w_mideleg(uint64_t x) {
    asm volatile("csrw mideleg, %0" : : "r"(x));
}

// Supervisor Trap-Vector Base Address
// low two bits are mode.
static inline void asm_w_stvec(uint64_t x) {
    asm volatile("csrw stvec, %0" : : "r"(x));
}

static inline uint64_t asm_r_stvec() {
    uint64_t x;
    asm volatile("csrr %0, stvec" : "=r"(x));
    return x;
}

// Machine-mode interrupt vector
static inline void asm_w_mtvec(uint64_t x) {
    asm volatile("csrw mtvec, %0" : : "r"(x));
}

// use riscv's sv39 page table scheme.
#define SATP_SV39 (8L << 60)

#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64_t)pagetable) >> 12))

// supervisor address translation and protection;
// holds the address of the page table.
static inline void asm_w_satp(uint64_t x) {
    asm volatile("csrw satp, %0" : : "r"(x));
}

static inline uint64_t asm_r_satp() {
    uint64_t x;
    asm volatile("csrr %0, satp" : "=r"(x));
    return x;
}

// Supervisor Scratch register, for early trap handler in trampoline.S.
static inline void asm_w_sscratch(uint64_t x) {
    asm volatile("csrw sscratch, %0" : : "r"(x));
}

static inline void asm_w_mscratch(uint64_t x) {
    asm volatile("csrw mscratch, %0" : : "r"(x));
}

// Supervisor Trap Cause
static inline uint64_t asm_r_scause() {
    uint64_t x;
    asm volatile("csrr %0, scause" : "=r"(x));
    return x;
}

// Supervisor Trap Value
static inline uint64_t asm_r_stval() {
    uint64_t x;
    asm volatile("csrr %0, stval" : "=r"(x));
    return x;
}

// Machine-mode Counter-Enable
static inline void asm_w_mcounteren(uint64_t x) {
    asm volatile("csrw mcounteren, %0" : : "r"(x));
}

static inline uint64_t asm_r_mcounteren() {
    uint64_t x;
    asm volatile("csrr %0, mcounteren" : "=r"(x));
    return x;
}

// supervisor-mode cycle counter
static inline uint64_t asm_r_time() {
    uint64_t x;
    // asm volatile("csrr %0, time" : "=r" (x) );
    // this instruction will trap in SBI
    asm volatile("rdtime %0" : "=r"(x));
    return x;
}

// enable device interrupts
static inline void intr_on() {
    asm_w_sstatus(asm_r_sstatus() | SSTATUS_SIE);
}

// disable device interrupts
static inline void intr_off() {
    asm_w_sstatus(asm_r_sstatus() & ~SSTATUS_SIE);
}

// are device interrupts enabled?
static inline int intr_get() {
    uint64_t x = asm_r_sstatus();
    return (x & SSTATUS_SIE) != 0;
}

static inline uint64_t asm_r_sp() {
    uint64_t x;
    asm volatile("mv %0, sp" : "=r"(x));
    return x;
}

// read and write tp, the thread pointer, which holds
// this core's hartid (core number), the index into cpus[].
static inline uint64_t asm_r_tp() {
    uint64_t x;
    asm volatile("mv %0, tp" : "=r"(x));
    return x;
}

static inline void asm_w_tp(uint64_t x) {
    asm volatile("mv tp, %0" : : "r"(x));
}

static inline uint64_t asm_r_ra() {
    uint64_t x;
    asm volatile("mv %0, ra" : "=r"(x));
    return x;
}

// flush the TLB.
static inline void sfence_vma() {
    // the zero, zero means flush all TLB entries.
    // asm volatile("sfence.vma zero, zero");
    asm volatile("sfence.vma");
}