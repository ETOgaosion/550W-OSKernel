kernel_irq_dir		:= irq
kernel_lock_dir		:= lock
kernel_mm_dir		:= mm
kernel_sched_dir	:= sched
modules		:= $(shell ls -d */)

.PHONY:	build clean $(modules)

build: $(modules)

$(modules):
	$(MAKE) build --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~

include ../../Makefile.in
