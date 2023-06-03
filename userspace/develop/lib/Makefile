INCLUDES := -I../include

target = $(shell ls *.c *.S | awk '{gsub(/\.c|\.S/, ".o"); print $0}')

.PHONY: all build clean

all: build

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../../Makefile.in