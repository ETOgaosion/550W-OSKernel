src_dir			:= src
linker_dir		:= linker
user_dir		:= userspace
target_dir		:= target
# tools_dir	 := tools

linkscript		:= $(linker_dir)/riscv.lds
vm550w_img		:= $(target_dir)/vm550w.img
vm550w_bin		:= kernel-qemu
vm550w_asm		:= $(target_dir)/vm550w_asm.txt
dst				:= /mnt
fs_img			:= sdcard.img

modules := $(src_dir)
objects := $(shell find $(src_dir) -name "*.o")

.PHONY: build clean $(modules) run
GDB := n

ifeq ($(MAKECMDGOALS), gdb)
	GDB = y
endif

all : clean build

build: $(modules)
	mkdir -p $(target_dir)
	$(LD) $(LDFLAGS) -T $(linkscript) -o $(vm550w_img) $(objects)
	$(OBJDUMP) -S $(vm550w_img) > $(vm550w_asm)
	$(OBJCOPY) -O binary $(vm550w_img) $(vm550w_bin)

sifive: clean build
	$(OBJCOPY) -O binary $(vm550w_img) /srv/tftp/vm.bin
	
fat: $(user_dir)
	if [ ! -f "$(fs_img)" ]; then \
		echo "making fs image..."; \
		dd if=/dev/zero of=$(fs_img) bs=512k count=1024; fi
	mkfs.vfat -F 32 $(fs_img); 
	sudo mount $(fs_img) $(dst)
	sudo cp -r userspace/rootfs/* $(dst)/
	sudo umount $(dst)

umount:
	sudo umount $(dst)

mount:
	sudo mount -t vfat $(fs_img) $(dst)
	
$(modules):
	$(MAKE) build GDB=$(GDB) --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~ $(target_dir)/*

comp: clean build
	$(QEMU) -kernel $(vm550w_bin) $(BOARDOPTS)

run: clean build
	$(QEMU) -kernel $(vm550w_bin) $(QEMUOPTS)

asm: clean build
	$(QEMU) -kernel $(vm550w_bin) $(QEMUOPTS) -d in_asm

int: clean build
	$(QEMU) -kernel $(vm550w_bin) $(QEMUOPTS) -d int

gdb: clean build
	$(QEMU) -kernel $(vm550w_bin) $(QEMUOPTS) -nographic -s -S

include Makefile.in
