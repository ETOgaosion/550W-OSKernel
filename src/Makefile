os_arch_dir     := arch/riscv
os_drivers_dir  := drivers
os_fs_dir       := fs
os_init_dir     := init
os_kernel_dir   := kernel
os_libs_dir     := libs
modules         := $(os_arch_dir) ${os_drivers_dir} ${os_fs_dir} ${os_init_dir} ${os_kernel_dir} ${os_libs_dir}

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

include ../Makefile.in
