user_lib_dir	:= lib
user_src_dir	:= src
user_preliminary_dir	:= src/preliminary
modules			:= $(user_lib_dir) ${user_src_dir}

link_script := linker/user_riscv.lds

lib_files := $(shell find $(user_lib_dir) \( -name "*.S" -o -name "*.c" \))
lib_objs := $(patsubst %.S,%.o, $(patsubst %.c,%.o, $(lib_files)))
lib_target := build/lib.a

src_files := $(shell find $(user_preliminary_dir) \( -name "*.c" -not -name "shell.c" \))
src_objs := $(patsubst %.c,%.o, $(src_files))
src_targets := $(patsubst %,%, $(basename $(notdir $(src_files))))
src_names := $(basename $(notdir $(src_files)))

shell_file = src/shell.c
shell_objs := src/shell.o
shell_target := build/shell
shell_asm := shell.txt

C_SHARE_FLAG :=  -O0 -nostdlib -T $(link_script) -Wall -nostdinc -g -ggdb -fvar-tracking

.PHONY: all build clean $(modules)

all: prepare

prepare: build $(lib_target) srcs $(shell_target) asm
	cp build/* ../rootfs/

asm:
	for prog in $(src_names); do $(OBJDUMP) -d build/$$prog > ../../target/$$prog.txt; done
	$(OBJDUMP) -d build/shell > ../../target/shell.txt

$(lib_target):
	$(AR) rcs $@ $(lib_objs)

srcs:
	for src_obj in $(src_names); do ${GCC} ${LDFLAGS} ${C_SHARE_FLAG} -o build/$$src_obj src/preliminary/$$src_obj.o ${lib_target}; done

$(shell_target):
	$(GCC) $(LDFLAGS) $(C_SHARE_FLAG) -o $@ ${shell_objs} ${lib_target}

build: $(modules)
	mkdir -p build

$(modules):
	$(MAKE) all --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~
	rm -rf build/*

include ../../Makefile.in
