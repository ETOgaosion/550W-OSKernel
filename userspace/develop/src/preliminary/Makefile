INCLUDES := -I../../include

target = $(shell ls *.c | awk '{gsub(/\.c/, ".o"); print $0}')

.PHONY: all build clean

all: build

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../../../Makefile.in