qemu-system-riscv64 -machine virt,dumpdtb=virt.out -kernel kernel-qemu -m 128M -nographic -smp 2 -drive file=sdcard.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
dtc -I dtb -O dts virt.out > virt_tree.txt
rm virt.out