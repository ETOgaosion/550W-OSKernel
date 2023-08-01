# 测试支持情况

现阶段支持的应用包括busybox, lua, libc-test等，对于其余部分测试仍需要更多的实验与系统调用支持。

# 部分BUG调试和心得体会

## 区域赛

### 由于fork得到的父子进程执行顺序不同而产生不同行为

```C
// test_clone.c
// fork();
if (child_pid == 0) {
    exit(0);
} else {
    if (wait(&wstatus) == child_pid) {
        printf("clone process successfully.\npid:%d\n", child_pid);
    } else {
        printf("clone process error.\n");
    }
}
```

考虑到fork之后得到的父子进程的执行顺序不同，父进程调用wait也没有给定pid，而如果子进程先于父进程wait时退出，那么理论上父进程的wait会采集不到需要等待的子进程的信息。

解决办法是让子进程退出时保留一定的状态信息，等待父进程调用wait再完全退出，类似于进入一种ZOMBIE状态。

## 全国赛第一阶段

### BUG1：load_elf的地址不对齐

现象：某个关键变量总是为0

对比：标准测试该变量应为非零值

debug流程：反复对比标准测试的变量赋值来源，发现是初始值，考虑到初始的数据段由load_elf加载，因此通过gdb检查load_elf对该数据段地址的处理。经过检查发现该地址为0x1000不对齐的地址，而原来的load_elf默认了地址都是对齐的，这样在借助虚地址申请新的物理页并建立映射时，很有可能将已有的映射覆盖掉，从而覆盖掉关键数据。

修正：为了方便数据加载，对于不对齐的地址，我们初始分配2页的物理空间，然后从虚地址开始加载一页的数据，下一次加载接着分配下一页的物理空间，避免覆盖当前已有的映射和数据。

```c
// elf.c
unsigned char *bytes_of_page = NULL;
if (phdr->p_vaddr & ((1 << NORMAL_PAGE_SHIFT) - 1)) {
    if (i == 0) {
        bytes_of_page = (unsigned char *)prepare_page_for_va((uintptr_t)(phdr->p_vaddr + i), pgdir);
        last_page = (unsigned char *)prepare_page_for_va((uintptr_t)(phdr->p_vaddr + i + NORMAL_PAGE_SIZE), pgdir);
    } else {
        bytes_of_page = last_page;
        last_page = (unsigned char *)prepare_page_for_va((uintptr_t)(phdr->p_vaddr + i + NORMAL_PAGE_SIZE), pgdir);
    }
} else {
    bytes_of_page = (unsigned char *)prepare_page_for_va((uintptr_t)(phdr->p_vaddr + i), pgdir);
}
```

### BUG2：Busybox参数初始化的问题

这里以Busybox为例，分享我们面向汇编指令debug的心得。

由于Busybox的代码太多，因此我们只能通过汇编指令地址来一步步定位错误。首先我们定位到地址0xd88c0的指令出现错误，该指令为：

```
d88c0:	9702                	jalr	a4
```

其中跳转的地址a4出现异常。经过向上排查，追溯调用栈，我们找到了如下的汇编代码片段：

```
d8884:	008d3783          	ld	a5,8(s10)
d8888:	cff9                	beqz	a5,d8966 <__run_exit_handlers+0x164>
d888a:	da0dbc03          	ld	s8,-608(s11)
d888e:	17fd                	addi	a5,a5,-1
d8890:	00fd3423          	sd	a5,8(s10)
```

正常来讲，0xd8884指令读到的a5是1，这样到0xd888e这条指令之后，a5变成0，存到原来的地址后再回到0x8884，读出来a5就是0，那么在d8888就不会再跳转。但是我们错误的a5就会读到2，那么就会导致整个循环再重复一遍，从而造成错误。

经过继续排查变量地址的赋值，我们最终定位到main函数开始的参数a0，该参数决定了后续a5的值。因此我们定位到是入口函数的传参出现了问题。

检查kernel部分的代码，我们找到了init_pcb_stack，该函数会初始化进程的用户栈，并传递参数，而init_context_stack则默认了寄存器传参，从而造成了冲突。当我们解决了init_context_stack寄存器传参的问题后，BUG就迎刃而解了。

## 感想和心得体会

这一阶段的调试充满了挑战性。首先就是源代码太长且复杂，很难像区域赛一样通过简单的源码定位问题，只能一步一步调试，通过出错的地址来反推问题所在。这样的调试经历充满了挑战性，也提升了我们debug的能力。