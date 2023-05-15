IMAGE_PATH=image

if [ "$1" == "ci" ];
then
    qemu-system-riscv64 -nographic -machine virt -m 256M -kernel /u-boot/u-boot -drive if=none,format=raw,id=image,file=${IMAGE_PATH} -device virtio-blk-device,drive=image -smp 2
else
    qemu-system-riscv64 -nographic -machine virt -m 256M -kernel /home/stu/OSLab-RISC-V/u-boot/u-boot -drive if=none,format=raw,id=image,file=${IMAGE_PATH} -device virtio-blk-device,drive=image -smp 2
fi

qemu-system-riscv64 -M virt -m 256M -nographic \
    -bios default

