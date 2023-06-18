# CONTRIBUTING

(开发注意事项)

## 环境配置

运行`zsh ./script/setup.sh zsh`或`bash ./script/setup.sh`

## 代码风格

源文件命名采用lower_case_with_underscores

开发完成后一定要在使用`./script/format.sh`来格式化代码（注意不要用clang format格式化`.h, .S`的代码），本项目coding style基于llvm clang-format做了一些调整。注意使用`clang-format 15`以上的版本，可以按照`./script/setup.sh`运行。

全局变量头文件里都用`extern`，在`.c`中定义；其余需要共享的`#define`直接写，不用前缀；类型声明、函数等只能声明一次，在其余头文件中使用`extern`。

只有三个头文件目录，可在vscode includePath设置中配置：

```json
{
    "includePath": [
        "${workspaceFolder}/src",
        "${workspaceFolder}/src/include",
        "${workspaceFolder}/src/arch/riscv/include"
    ]
}
```

头文件的所有引用按照字母序排列，脚本可以辅助解决排序。头文件内的书写顺序为：

- `#pragma once`
- `#include`
- `#define`
- `typedef`
- `typedef struct`
- `extern`
- 函数按照依赖次序排列，同类函数放在一起，`.c`文件的实现中函数与头文件中函数顺序一致

系统调用全部使用`sys_`开头lower_case_with_underscores，系统调用号使用`SYS_`后面是lower_case_with_underscores。系统调用在各个子模块头文件中实现，而后将头文件加入./src/os/syscall.h。

内核函数标准模板：

```C
retval func_name(...) {
    int error = -EINVAL;
    if (/* args check or false statements */) {
        goto error;
    }
    /* var declarations */
    /* logics */
    /* don't forget to modify error */
func_name_out:
    /* some jobs */
    return error;
}
```

函数命名模板：

- 系统调用全部使用`long sys_xxx(args)`的命名形式，开始一定要进行参数检查，指针一定是用户地址空间。
- 内核内互相调用的函数，即在头文件中像内核其他模块暴露的函数全部以`k_`开头，效率起见可不做参数检查，参数可兼容用户态和内核态地址。
- 其余函数不要放在头文件中，在函数内定义，使用`static`保证安全，开头不能是`k_`和`sys_`

函数实现中尽量提取多个类似函数的共同操作组成`static`函数公用，减少代码量。

内核中共享的变量和类型定义最好也使用`k_`开头定义，如果函数有枚举作用的参数只在内核态范围交换则使用`enum`，如果是用户态传入则使用其他`int`或`long`类型。

### namespace

本项目虽然是C语言项目，但需要借助一些其它语言的高级特性来辅助开发和使代码美观，首先希望吸纳的是C++ namespace特性，将不同模块之间的函数隔离开，并且有助于层次化与接口的实现。

C风格命名空间实现为采用lower_case_with_underscores命名风格，将命名空间置于名字前方，来区分函数与变量所属空间。所需要重点关注的是需要共享的函数与变量，其余原则上也应该按此规范。

本项目顶层有以下命名空间：

- `asm`: 体系结构有关
- `sbi`: SBI相关
- `d`: drivers下不同函数
- `k`: kernel本身，体系结构无关功能
- `fs`: 文件系统函数
- `sys`: syscall函數，只负责参数检查，功能由内核函数完成

driver与kernel下按照组件和功能继续划分namespace，fs按照文件系统类型划分namespace。

### 错误检测与

### 注释与文档

特殊注释：`[TODO] :`本人待做，`[FUNCTION REQUIREMENTS]`其他人可提出需求，该模块负责人来实现

由于本项目采用Doxygen生成文档，`\todo`也是可以接受的

## 编译

使用`./script/build.sh`来编译

## 运行QEMU

使用`./script/qemu_run.sh`来运行

## QEMU Debug

使用`./script/qemu_debug.sh`来debug