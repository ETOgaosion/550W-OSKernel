os_arch_dir		:= arch/riscv
os_drivers_dir	:= drivers
os_fs_dir		:= fs
os_init_dir		:= init
os_kernel_dir	:= kernel
os_lib_dir		:= lib
os_user_dir		:= user		# tmp
os_tests_dir	:= tests
modules			:= $(os_arch_dir) ${os_drivers_dir} ${os_fs_dir} ${os_init_dir} ${os_kernel_dir} ${os_lib_dir} ${os_user_dir} ${os_tests_dir}

.PHONY:	build clean $(modules)

all: build

build: $(modules)

$(modules):
	$(MAKE) all --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~

include ../Makefile.in
