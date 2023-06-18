# OSKernel2023-550W

## Brief

Here is our [technical report](./docs/report.md).

We developed a full function kernel. We hope to make it have **Advanced Feelings**(高级感), so we will try hard to refactor again and again to make every loc perfect.

## For developers

**Read [CONTRIBUTING](./CONTRIBUTING) first when you start to develop.**

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
│   └── tests                   # kernel related tests, built into kernel
│   └── user                    # temp compromise
├── target                  # build target location
├── tests                   # non-kernel related tests, mainly for language features using host compiler and env
├── tools                   # OS-none-related tools
└── userspace               # userspace programs
```

## Copyrights

Copyrights belong to Contributors, you must include the copyright announcement when citing this work or use its codes.

We use Apache 2.0 License for source code, and CC-BY-SA 4.0 License for documentary.
