FROM ubuntu:18.04

USER root
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Aisa/Shanghai

RUN apt update && \
    apt install apt-transport-https ca-certificates && \
    update-ca-certificates
RUN sed -i s@/archive.ubuntu.com/@/mirrors.tuna.tsinghua.edu.cn/@g /etc/apt/sources.list && \
    apt update
RUN apt install -y --no-install-recommends \
        gcc libc-dev git build-essential gdb-multiarch \
        gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu \
        libpixman-1-0 git python3 python3-pip make curl \
        sshpass openssh-client clang-10 libtinfo5 libc6-dev-i386

RUN rm -rf /var/lib/apt/lists/*

RUN pip3 config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
RUN pip3 install pytz Cython jwt jinja2 requests

COPY env/kendryte-toolchain /opt/kendryte-toolchain

COPY env/qemu-prebuilt-7.0.0.tar.gz /qemu/qemu.tar.gz
RUN cd /qemu && tar xavf /qemu/qemu.tar.gz

ENV PATH "$PATH:/opt/kendryte-toolchain/bin"
ENV PATH "/qemu/bin/:$PATH"

ENV LD_LIBRARY_PATH /opt/kendryte-toolchain/bin/

