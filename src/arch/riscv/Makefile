asm_kernel_dir	:= kernel
asm_sbi_dir		:= sbi
modules		:= $(asm_kernel_dir) $(asm_sbi_dir)

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

include ../../../Makefile.in
