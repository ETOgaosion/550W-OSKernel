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
    sudo apt update && \
    sudo apt install -y apt-transport-https ca-certificates && \
    sudo update-ca-certificates
    sudo sed -i s@/archive.ubuntu.com/@/mirrors.tuna.tsinghua.edu.cn/@g /etc/apt/sources.list
    sudo apt update
    sudo apt install -y --no-install-recommends \
        gcc libc-dev git build-essential gdb-multiarch \
        gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu \
        libpixman-1-0 git python3 python3-pip make curl \
        sshpass openssh-client clang-10 libtinfo5 libc6-dev-i386 \
        libncurses5 libpython2.7 libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev
    curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | sudo bash
    sudo apt-get install git-lfs
    git lfs install
    pip3 config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
    pip3 install pytz Cython jwt jinja2 requests
else
    apt update && \
    apt install -y apt-transport-https ca-certificates && \
    update-ca-certificates
    sed -i s@/archive.ubuntu.com/@/mirrors.tuna.tsinghua.edu.cn/@g /etc/apt/sources.list
    apt update
    apt install -y --no-install-recommends \
        gcc libc-dev git build-essential gdb-multiarch \
        gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu \
        libpixman-1-0 git python3 python3-pip make curl \
        sshpass openssh-client clang-10 libtinfo5 libc6-dev-i386 \
        libncurses5 libpython2.7 libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev
    curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash
    apt-get install git-lfs
    git lfs install
    pip3 config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
    pip3 install pytz Cython jwt jinja2 requests
fi

echo "=== Clone Repository ==="
test -d env || mkdir env
if ! [[ -d $REPO ]]
then
    git clone $REMOTE env/$REPO
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

