# Usage:
# [bash|zsh] setup.sh [zsh]
echo "=== choose options ==="
echo "use sudo? [y/n]"
read -t 30 use_sudo_opt

echo "=== Config ==="
SHFILE=~/.bashrc
if [[ `echo $1` = "zsh" ]]
then
    SHFILE=~/.zshrc
    echo "yes"
fi
RISCV_DIR=/opt/kendryte-toolchain
QEMU_DIR=/qemu
REPO=os-contest-2023-image
REMOTE=https://gitlab.educg.net/wangmingjian/os-contest-2023-image.git

echo "SHFILE = $SHFILE"
echo "REPO = $REPO"
echo "RISCV_DIR = $RISCV_DIR"
echo "QEMU_DIR = $QEMU_DIR"
echo "=== Install dependencies ==="
if [[ "$use_sudo_opt" = "y" ]]
then
    sudo apt install libncurses5 libpython2.7 libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev -y
else
    apt install libncurses5 libpython2.7 libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev -y
fi

echo "=== Clone Repository ==="
if ! [[ -d $REPO ]]
then
    git clone $REMOTE $REPO
fi

echo "=== Install RISC-V Toolchain ==="

if ! [[ -d $OSTEST_DIR ]]
then
    mkdir -p $OSTEST_DIR
    mv -r $REPO/kendryte-toolchain $RISCV_DIR
    chmod -R $RISCV_DIR
fi

if ! grep -q "$RISCV_DIR" "$SHFILE"; then
    echo export PATH=$RISCV_DIR/bin:'$PATH' >> $SHFILE
    echo export LD_LIBRARY_PATH=$RISCV_DIR/bin:'$LD_LIBRARY_PATH' >> $SHFILE
fi

echo "Check Installation"
source $SHFILE
riscv64-unknown-elf-gcc --version

echo "=== Install QEMU ==="

if ! [[ -d $QEMU_DIR ]]
then
    mkdir -p $QEMU_DIR
    tar -xvf $REPO/qemu-prebuilt-7.0.0.tar.gz -C $QEMU_DIR
    rm $REPO/qemu-prebuilt-7.0.0.tar.gz
fi

if ! grep -q "$QEMU_DIR" "$SHFILE"; then
    echo export PATH=$QEMU_DIR/bin:'$PATH' >> $SHFILE
fi

echo "Check Installation"
source $SHFILE
qemu-system-riscv64 --version

echo "=== Clean Up ==="
echo "clean remote repo? [y/n]"
read -t 30 clean_repo
if [[ "$clean_repo" = "y" || -d $REPO ]]
then
    rm -r $REPO
fi

