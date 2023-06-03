src_dir			:= src
linker_dir		:= linker
tools_dir		:= tools
user_dir		:= userspace
target_dir		:= target

linkscript		:= $(linker_dir)/riscv.lds
target			:= kernel-qemu
vm550w_bin		:= $(target_dir)/vm550w.bin
vm550w_asm		:= $(target_dir)/vm550w_asm.txt
dst				:= /mnt
fs_img			:= sdcard.img

modules := $(tools_dir) $(user_dir) $(src_dir)
sources := $(shell find $(src_dir) \( -name "*.S" -o -name "*.c" \))
objects := $(patsubst %.S,%.o, $(patsubst %.c,%.o, $(sources)))

.PHONY: all build clean $(modules) run

all: clean target_dir build

target_dir:
	mkdir -p $(target_dir)

build: $(modules)
	mkdir -p $(target_dir)
	$(LD) $(LDFLAGS) -T $(linkscript) -o $(target) $(objects)
	$(OBJDUMP) -S $(target) > $(vm550w_asm)
	$(OBJCOPY) -O binary $(target) $(vm550w_bin)

sifive:
	$(OBJCOPY) -O binary $(target) /srv/tftp/vm.bin
	
fat:
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
	$(MAKE) all GDB=$(BUILD_DEBUG) --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~

comp:
	$(QEMU) -kernel $(target) $(BOARD_OPTS)

run:
	$(QEMU) -kernel $(target) $(QEMU_OPTS)

asm:
	$(QEMU) -kernel $(target) $(QEMU_OPTS) -d in_asm

int:
	$(QEMU) -kernel $(target) $(QEMU_OPTS) -d int

qemu:
	$(QEMU) -kernel $(target) $(QEMU_OPTS) -nographic -s -S

debug: qemu

gdb:
	$(GDB)

include Makefile.in
