asm_kernel_dir    := kernel
asm_sbi_dir        := sbi
asm_syscall_dir    := syscall
asm_modules        := $(asm_kernel_dir) $(asm_sbi_dir) $(asm_syscall_dir)

.PHONY:    build clean $(modules)

build: $(modules)

$(modules):
    $(MAKE) build --directory=$@

clean:
    for d in $(modules); \
        do \
            $(MAKE) --directory=$$d clean; \
        done
    rm -rf *.o *~

include ../../../Makefile.in
