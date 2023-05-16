INCLUDES := -I../../../../include -I../../include

target = $(shell ls *.S | awk '{gsub(/\.S/, ".o"); print $0}')

.PHONY: build clean

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../../../../Makefile.in