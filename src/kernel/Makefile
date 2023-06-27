kernel_irq_dir		:= irq
kernel_sync_dir		:= sync
kernel_socket_dir	:= socket
kernel_mm_dir		:= mm
kernel_sys_dir		:= sys
kernel_users_dir	:= users
modules		:= $(shell ls -d */)

.PHONY:	build clean $(modules)

all: build

build: $(modules) $(target)

$(modules):
	$(MAKE) all --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *~ *.o *.d

include ../../Makefile.in
