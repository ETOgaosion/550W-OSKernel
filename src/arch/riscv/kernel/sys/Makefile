INCLUDES := -I../../../../include -I../../include

target = $(shell ls *.c *.S | awk '{gsub(/\.c|\.S/, ".o"); print $0}')

.PHONY: all build clean

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

all: build

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../../../../Makefile.in