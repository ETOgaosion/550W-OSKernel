kernel_irq_dir		:= irq
kernel_lock_dir		:= lock
kernel_mm_dir		:= mm
kernel_sched_dir	:= sched
modules		:= $(shell ls -d */)

INCLUDES := -I../include -I../arch/riscv/include -I../

target = $(shell ls *.c | awk '{gsub(/\.c/, ".o"); print $0}')

.PHONY:	build clean $(modules)

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(modules) $(target)

$(modules):
	$(MAKE) build --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *~ *.o *.d

include ../../Makefile.in
