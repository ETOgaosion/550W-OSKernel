# 启动与初始化

本节主要介绍内核的启动和初始化流程

## 启动流程

```sh
OpenSBI完成必要的流程后，抵达地址0x80200000
                    ↓
从0x80200000进入head.S，拷贝当前CPU核编号，进行中断屏蔽等，
然后call prepare_vm
                    ↓
prepare_vm会建立并开启虚地址映射。之后call main，进入内核主函数
                    ↓
main函数中会初始化中断表，初始化进程数据结构PCB，初始化virtio，
初始化文件系统等等，之后设置时钟中断并通过k_scheduler开启任务调度
```

## 初始化

初始化操作主要在main.c中进行，下面简单介绍其中一部分初始化流程

### PCB初始化

k_init_pcb主要清空PCB数组，把pid设置为-1表示无效，把status设置为EXITED。然后初始化current_running以及timers链表（用于sleep）。

### 中断表和系统调用表初始化

在init_exception中初始化中断表，为每个中断号填充中断处理函数。如时钟中断，系统调用的例外处理等等。

在init_syscall中初始化系统调用表，为每个系统调用号填充系统调用函数。