drivers_screen_dir		:= screen
drivers_plic_dir		:= plic
drivers_virtio_dir		:= virtio
modules		:= $(drivers_screen_dir) $(drivers_plic_dir) $(drivers_virtio_dir)

.PHONY:	build clean $(modules)

all: build

build: $(modules)

$(modules):
	$(MAKE) all --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~

include ../../Makefile.in
