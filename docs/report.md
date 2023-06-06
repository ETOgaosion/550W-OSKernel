# 操作系统大赛技术报告（区域赛）

> 设计思路、系统框架、实现重点、代码注释（中英文均可）、开发过程中遇到的主要问题和解决方法。

## 基本情况

我们设计的操作系统内核是在参考了 Linux 的基础上，从零重新设计的一个内核。支持启动初始化、中断、I/O、进程管理、内存管理、执行文件解析、文件系统功能等。

## 系统框架

我们的系统框架受到 Linux 系统的启发，具体如下：

```sh
550W
├── docs                    # 技术报告
├── env                     # 编译环境
├── linker                  # 链接文件
├── mnt                     # 
├── scripts                 # 脚本
├── src                     # 源文件
│   ├── arch                # 汇编代码
│   │   └── riscv           # riscv64
│   │       ├── include     # 包含了汇编代码需要的头文件
│   │       │   ├── asm     # asm头文件
│   │       │   └── uapi                    # uapi for outsiders' usage, like syscall number
│   │       ├── kernel                  # kernel related arch-specified code
│   │       ├── sbi                     # sbi code
│   │       └── syscall                 # arch-specified syscall implementation
│   ├── drivers                 # drivers
│   ├── fs                      # file system, as independent module from kernel
│   ├── include                 # arch-independent headers
│   │   ├── common                  # common data structure headers
│   │   ├── lib                     # library functions headers
│   │   └── os                      # OS/kernel headers
│   ├── init                    # initiate process
│   ├── kernel                  # kernel core code
│   │   ├── irq                     # deal with interrupt and exeptions
│   │   ├── mm                    # deal with locking
│   │   ├── sync                      # deal with memory management
│   │   └── sys                   # deal with scheduling
│   ├── libs                    # libraries for kernel use only, like string and print
│   └── tests                   # kernel related tests, built into kernel
│   └── user                    # temp compromise
├── target                  # build target location
├── tests                   # non-kernel related tests, mainly for language features using host compiler and env
├── tools                   # OS-none-related tools
└── userspace               # userspace programs
```

## 实现重点

### [启动与初始化](./details/boot.md)

由于OpenSBI让CPU从地址0x80200000开始运行，为了方便启动内核，我们直接使用call进行内核启动。

内核启动前进行虚页的初始化。内核启动后，进行PCB管理结构、中断、磁盘和文件系统等的初始化。

### [中断](./details/interrupt.md)

我们在内核启动时进行中断的初始化，中断处理流程为标准的保存上下文->执行中断处理->恢复上下文。

### [I/O](./details/io.md)

我们通过SBI调用实现Console的IO操作，通过实现virtio来对磁盘进行IO操作。

### [进程管理](./details/process_management.md)

我们使用PCB数据结构统一管理进程和线程。我们实现了基于时钟中断的抢占式调度。

### [内存管理](./details/memory_management.md)

为了便于实现，当前我们采用的是简单的顺序分配和回收机制。预计未来将实现伙伴分配算法等更复杂高效的内存管理系统。

### [执行文件解析](./details/file_analysis.md)

不懂

### [文件系统功能](./details/file_system.md)

不懂

## 主要问题与解决方法

### test_pipe 测试点的问题
该测试点中，调用 `fork` 后打印了返回值：
```c
cpid = fork();
printf("cpid: %d\n", cpid);
```

并将该输出作为评分之一：
```
cpid: 0
cpid: 28
```

但是，由于 `fork` 之后，父进程和子进程的执行顺序是不确定的，抢占的时机也是不确定的，因此可能出现比较混乱的输出，例如：
```
cpid: cpid: 028
```

这样的输出不能得到测试点的分数，但我们认为是正常情况，也许应该：
- 将 `fork` 之后的输出作为评分之一，但不要求输出顺序；或不作为评分依据
- 在 `fork` 后加入 `wait` 保证一方先执行

## 后续开发计划

## 队伍信息

队名：550W

学校：中国科学院大学

成员：张玉龙 高梓源 李盛开

指导教师：王晨曦 王卅