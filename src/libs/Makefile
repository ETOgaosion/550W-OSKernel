INCLUDES := -I../include -I../arch/riscv/include -I../

target = $(shell ls *.c | awk '{gsub(/\.c/, ".o"); print $0}')

.PHONY: build clean

%.o: %.c
    $(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
    rm -rf *~ *.o *.d

include ../../Makefile.in