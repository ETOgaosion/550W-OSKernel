# 系统框架

## 代码结构图

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
│   │       │   └── asm                     # asm headers
│   │       ├── kernel                  # kernel related arch-specified code
│   │       │   ├── init                    # only use when initializing kernel
│   │       │   └── sys                     # system-wide usage
│   │       └── sbi                     # sbi code
│   ├── drivers                 # drivers
│   ├── fs                      # file system, as independent module from kernel
│   ├── include                 # arch-independent headers
│   │   ├── common                  # common data structure headers
│   │   ├── lib                     # library functions headers
│   │   ├── os                      # OS/kernel headers
│   │   └── uapi                    # uapi for outsiders' usage, like syscall number
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

## 设计理念

Linux内核代码框架的设计最大的特点就是满足模块化和接口的设计，这对于内核的开发是极其重要的。传统C程序构建的内核的缺点是模块耦合度较高，面向过程的特性占据主流，这非常不适合现代软件开发理念和合作开发的要求，在我们小组实践的过程中，由于代码耦合性导致的合并冲突不占少数。因此如何将内核按照模块化、接口化的设计使得不同功能的耦合性尽量降低，使得每个团队成员能够无摩擦地顺利协作完成各个流程的功能、为单一流程设计不同的解决方案。

和代码的并行优化类似，团队开发也需要尽可能实现并行，低耦合的特性是至关重要的，只有功能之间隔离性足够高、一个功能的变更不会影响其他模块才能够使得每个团队成员开发与测试时能够高效解决自己所遇到的问题，而非花时间与精力与其他开发者协调。

## 解释

可以发现src目录下结构和Linux较为相似，经过一系列思考与理解，几乎每个目录结构的放置位置和命名都是有说法的：

- arch: 体系结构相关代码与体系结构无关代码完全隔离，以函数调用和编译宏的方式实现面向对象中的**接口**，不同的体系结构细节可以来实现这些接口
- drivers: 驱动目录，设计是按照类型将驱动进行整合，如块设备、网络设备等，每种驱动类型由不同设备的不同驱动完成统一的驱动动作
- fs: 文件系统，设计是实现虚拟文件系统的概念，将文件系统的通用结构抽象出来，根据情况对接不同类型的文件系统底层实现
- include: 除上述目录外所有源文件所需头文件的集合，按功能划分为常用约定、库函数、系统、用户接口。这一层提取是为了避免实现变复杂以后头文件变得混乱
- init: 启动流程代码，因为本项目采用面向对象的设计而非面向过程，kernel内核本质上是**面向功能的**，即面向系统调用实现的，而启动流程不属于这种功能，将其独立出来
- kernel: 内核所有功能，包括中断处理、同步操作、内存管理、系统功能实现，事实上Linux中内存管理也被单独拆分出内核，但笔者认为内存管理是一种强耦合的内核功能，并未拆分
- lib: 库文件实现，仅供内核使用
- tests: 内核功能测试，将一同编译执行
- user: 读去磁盘中用户程序的模块

其余目录与文件都起到很大的开发辅助作用，比如辅助全流程编译、测试、代码格式化等。