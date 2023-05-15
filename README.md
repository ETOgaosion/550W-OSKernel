# OSKernel2023-550W

We develop a full function kernel.

**Read [docs/development](./docs/development.md) first when you start to develop.**

Our Framework is inspired from Linux, as below:

```sh
.
├── docs                    # documentary
├── linker                  # riscv link scripts for kernel and user program
├── output                  # building output
├── scripts                 # shell scripts as development assistance
├── src                     # OS source code
│   ├── arch                    # architecture specified code
│   │   └── riscv                   # riscv64
│   │       ├── include                 # including directories referenced by kernel and implementd by arch-code
│   │       │   ├── asm                     # asm headers including definations or macros
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
│   │   ├── lock                    # deal with locking
│   │   ├── mm                      # deal with memory management
│   │   └── sched                   # deal with scheduling
│   ├── libs                    # libraries for kernel use only, like string and print
│   └── user                    # temp compromise
├── target                  # build target location
├── test                    # test of OS modules
├── tools                   # OS-none-related tools
└── userspace               # userspace programs
```
