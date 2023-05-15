IMAGE_PATH=image

if [ "$1" == "ci" ];
then
    qemu-system-riscv64 -nographic -machine virt -m 256M -kernel /u-boot/u-boot -drive if=none,format=raw,id=image,file=${IMAGE_PATH} -device virtio-blk-device,drive=image -smp 2
else
    qemu-system-riscv64 -nographic -machine virt -m 256M -kernel /home/stu/OSLab-RISC-V/u-boot/u-boot -drive if=none,format=raw,id=image,file=${IMAGE_PATH} -device virtio-blk-device,drive=image -smp 2
fi

qemu-system-riscv64 -machine virt -kernel kernel-qemu -m 128M -nographic -smp 2 -bios sbi-qemu -drive file=sdcard.img,if=none,format=raw,id=x0  -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -initrd initrd.img

