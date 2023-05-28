targets = $(shell ls *.c | awk '{gsub(/\.c/, ""); print $0}')

C_TOOL_FLAG := -g -ggdb -Wall

.PHONY: all build clean

all: build

build:
	for target in $(targets); do ${HOST_CC} ${C_TOOL_FLAG} -o $$target $$target.c; done

clean:
	rm -rf *~ $(targets)

include ../Makefile.in