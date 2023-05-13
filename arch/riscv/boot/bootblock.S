#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x502001f0

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50301000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50301000

.text
.global main


main:
	// The core whose mhartid=0 is selected to boot the kernel.
	// Other cores should wait until the kernel's initialization
	// has been finished.
  	addi s0, a0, 0
	bnez a0, secondary
	.option push
  	.option norelax
  	la gp, __global_pointer$
  	.option pop
	j L2
L1:
	addi t0, a0, 0
	li t3, (0x8000)
	addi t1, a1, 0
	li t2, 64
	bge t1, t2, L5
	addi a1, t1, 0
	SBI_CALL SBI_SD_READ
	fence.i
	lui s1, %hi(kernel_main)
	addi s1, s1, %lo(kernel_main)
	jr s1
L5:
	li a1, 64
	SBI_CALL SBI_SD_READ
	fence.i
	addi a2, a2, 64
	addi t1, t1, -64
	add  a0, t0, t3
	addi t0, a0, 0
	bge t1, t2, L5
	addi a1, t1, 0
	SBI_CALL SBI_SD_READ
	fence.i	
	lui s1, %hi(kernel_main)
	addi s1, s1, %lo(kernel_main)
	jr s1
L2:
	// fence on all memory and I/O
	fence

	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's bootblock!"
	la a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR

	//t0(-1),t1(48),t2(0),t4(os_size_loc),t3(os2_size)
	li t0, -1
	li t1, 48
	li t2, 0
	lui t4, %hi(os_size_loc)
	addi t4, t4, %lo(os_size_loc)
	lh t3, 2(t4)
	//if os2_size==0, only one OS
	beq t3, t2, Os

	//or, two OS, print welcome
	la a0, wel
	SBI_CALL SBI_CONSOLE_PUTSTR
loop:
	SBI_CALL SBI_CONSOLE_GETCHAR
	//no input, continue getchar
	beq a0, t0, loop
	//input '0', OS0 start
	beq a0, t1, Os0 
Os1:
	//for OS1: t2(os1_size), t4=(os2_size_loc)
	lh t2, (t4) 
	addi t4, t4, 2
Os0:
	//print what user typed
	addi t0, a0, 0
	la a0, msg2
	SBI_CALL SBI_CONSOLE_PUTSTR
	addi a0, t0, 0
	SBI_CALL SBI_CONSOLE_PUTCHAR
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR
Os:
	//a0(kernel_address),a2(t2+1)
	lui a0, %hi(kernel)
	addi a0, a0, %lo(kernel)
	addi a2, t2, 1
	//t2(u_os_size),a1(u_os_size)
	lh t2, (t4)
	addi a1, t2, 0
	//t2(the last address of kernel),t1(512),t5(t2)
	/*add t2, a1, a2
	li t1, 512
	mul t2, t2, t1
	add t2, a0, t2
	addi t5, t2, 0

	// t1(L1), t3(L2), t0=data on (t1)
	la t1, L1
	la t3, L2
L4:
	bge t1, t3, L3
	lb t0, (t1)
	sb t0, (t2)
	addi t1, t1, 1
	addi t2, t2, 1
	j L4
L3:	*/
	lui s1, %hi(kernel_main)
	addi s1, s1, %lo(kernel_main)
	//jr t5
	j L1

secondary:
	/* TODO: 
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */
	csrw CSR_SIE, zero
  	csrw CSR_SIP, zero
	lui t0, %hi(kernel_main)
	addi t0, t0, %lo(kernel_main)
  	csrw CSR_STVEC, t0
	li t0, 0x2
  	csrw CSR_SIE, t0
  	li t0, SR_SIE
  	csrw CSR_SSTATUS, t0

wait_for_wakeup:
	wfi
	j wait_for_wakeup

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's Boot Loader!\n\r"
wel: .string "Choose your OS by typing:\n\r0: OS0\n\r1 or other numbers: OS1\n\r"
msg2: .string "Your choice is: "