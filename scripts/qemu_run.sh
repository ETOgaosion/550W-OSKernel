qemu-system-riscv64 \
    -machine virt \
    -kernel kernel-qemu \
    -m 128M \
    -nographic \
    -smp 2 \
    -bios default \
    -drive file=sdcard.img,if=none,format=raw,id=x0 \
    -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0