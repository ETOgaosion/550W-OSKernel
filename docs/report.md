# 操作系统大赛技术报告

*对技术报告的目录进行了重构，有更新的内容放到了决赛一阶段目录下，决赛一阶段技术报告位于[details/final_1](details/final_1/)*

## 基本情况

我们设计的操作系统内核是在参考了以Linux为主的先进操作系统的基础上，精心设计的一个全功能操作系统，支持启动初始化、中断、I/O、进程管理、内存管理、执行文件解析、文件系统功能等。

## 系统框架

我们系统框架的设计受到 Linux 系统的启发，具体如下：

```sh
550W
├── docs                    # technical report
├── env                     # compilation and testing environment
├── linker                  # linker scripts
├── mnt                     # sdcard mount point
├── scripts                 # scripts for developing assistance
├── src                     # source codes
│   ├── arch                    # architecture specified codes
│   │   └── riscv                   # riscv64
│   │       ├── include                 # architecture related headers
│   │       │   ├── asm                     # asm headers
│   │       │   └── uapi                    # uapi for outsiders' usage, like syscall number
│   │       ├── kernel                  # kernel related arch-specified code
│   │       ├── sbi                     # sbi code
│   ├── drivers                 # drivers
│   ├── fs                      # file system, as independent module from kernel
│   ├── include                 # arch-independent headers
│   │   ├── common                  # common data structure headers
│   │   ├── lib                     # library functions headers
│   │   └── os                      # OS/kernel headers
│   ├── init                    # initiate process
│   ├── kernel                  # kernel core code
│   │   ├── irq                     # deal with interrupt and exeptions
│   │   ├── mm                      # deal with memory management
│   │   ├── socket                  # deal with socket function
│   │   ├── sync                    # deal with synchronize operations like semaphores
│   │   ├── sys                     # deal with system core functions like pcb management
│   │   └── users                   # deal with user management
│   ├── libs                    # libraries for kernel use only, like string and print
│   └── tests                   # kernel related tests, built into kernel
│   └── user                    # user programs loader
├── target                  # build target location
├── tests                   # non-kernel related tests, mainly for language features using host compiler and env
├── tools                   # OS-none-related tools
└── userspace               # userspace programs
```

在[这一章节中](./details/preliminary/architecture.md)我们详细地讨论了架构与框架的设计理念。

## 实现重点

本节各个小标题均链接有位于details文件夹下的详细介绍说明。

### [启动与初始化](./details/preliminary/boot.md)

由于OpenSBI让CPU从地址0x80200000开始运行，为了方便启动内核，我们直接使用call进行内核启动。

内核启动前进行虚页的初始化。内核启动后，进行PCB管理结构、中断、磁盘和文件系统等的初始化。

### [中断](./details/preliminary/interrupt.md)

我们在内核启动时进行中断的初始化，中断处理流程为标准的保存上下文->执行中断处理->恢复上下文。

### [I/O](./details/preliminary/io.md)

我们通过SBI调用实现Console的IO操作，通过实现virtio来对磁盘进行IO操作。

### [进程管理](./details/final_1/process_management.md)

我们使用PCB数据结构统一管理进程和线程。

我们实现了基于时钟中断的抢占式调度。

### [内存管理](./details/final_1/memory_management.md)

为了便于实现，当前我们采用的是简单的顺序分配和内存回收机制。

在虚存部分，我们实现了按需分配物理地址的模式，同时支持了fork的copy-on-write机制。

预计未来将实现伙伴分配算法等更复杂高效的内存管理系统。

### [执行文件解析](./details/preliminary/file_analysis.md)

结合fat32文件系统格式，进行文件解析，详见文档。

### [文件系统功能](./details/final_1/file_system.md)

实现了基本磁盘解析，文件描述符和系统调用，详见文档。

### [测试相关](./details/final_1/tests.md)

实现了测试的支持，该章节也介绍了我们遇到的一些BUG和解决方法

### [虚拟文件系统](./details/final_1/vfs.md)

本项目并未完全引入VFS来对各种FS进行抽象，这里所谓`虚拟`，指的是并不存在于物理文件系统内的`虚拟文件和目录`，当用户对其发起读写等操作时将转化为调用内核函数。

## 队伍信息

队名：550W

学校：中国科学院大学

成员：张玉龙 高梓源 李盛开

指导教师：王晨曦 王卅