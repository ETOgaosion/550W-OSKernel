user_lib_dir	:= lib
user_src_dir	:= src/preliminary
modules			:= $(user_lib_dir) ${user_src_dir}

link_script := linker/user_riscv.lds

lib_files := $(shell find $(user_lib_dir) \( -name "*.S" -o -name "*.c" \))
lib_objs := $(patsubst %.S,%.o, $(patsubst %.c,%.o, $(lib_files)))
lib_target := build/lib.a

src_files := $(shell find $(user_src_dir) \( -name "*.c" -not -name "shell.c" \))
src_objs := $(patsubst %.c,%.o, $(src_files))
src_targets := $(patsubst %,%.elf, $(basename $(notdir $(src_files))))
src_names := $(basename $(notdir $(src_files)))

shell_file = src/shell.c
shell_objs := src/shell.o
shell_target := build/shell.elf
shell_asm := shell.txt

C_SHARE_FLAG :=  -O0 -nostdlib -T $(link_script) -Wall -nostdinc -g -ggdb -fvar-tracking

.PHONY: all build clean $(modules)

all: prepare asm

prepare: build $(lib_target) srcs $(shell_target)
	echo "" > user_programs.c
	echo "" > user_programs.h
	../../tools/elf2char -o shell --header-only $(shell_target) >> user_programs.h
	../../tools/elf2char -o shell $(shell_target) >> user_programs.c
	for prog in $(src_names); do ../../tools/elf2char -o $$prog --header-only build/$$prog.elf >> user_programs.h; done
	for prog in $(src_names); do ../../tools/elf2char -o $$prog build/$$prog.elf >> user_programs.c; done
	../../tools/generateMapping user_programs
	mv user_programs.h ../../src/user/
	mv user_programs.c ../../src/user/

asm:
	for prog in $(src_names); do $(OBJDUMP) -d build/$$prog.elf > ../../target/$$prog.txt; done
	$(OBJDUMP) -d build/shell.elf > ../../target/shell.txt

$(lib_target):
	$(AR) rcs $@ $(lib_objs)

srcs:
	for src_obj in $(src_names); do ${GCC} ${LDFLAGS} ${C_SHARE_FLAG} -o build/$$src_obj.elf src/preliminary/$$src_obj.o ${lib_target}; done

$(shell_target):
	$(GCC) $(LDFLAGS) $(C_SHARE_FLAG) -o $@ ${shell_objs} ${lib_target}

build: $(modules)
	mkdir -p all

$(modules):
	$(MAKE) build --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~
	rm -rf build/*

include ../../Makefile.in
