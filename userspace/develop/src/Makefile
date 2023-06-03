INCLUDES := -I../include

user_premliminary_dir := preliminary
modules		:= $(user_premliminary_dir)

target = $(shell ls *.c | awk '{gsub(/\.c/, ".o"); print $0}')

.PHONY: all build clean $(modules)

all: build

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target) $(modules)

$(modules):
	$(MAKE) all --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~

include ../../../Makefile.in