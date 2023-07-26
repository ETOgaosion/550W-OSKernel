# 虚拟文件系统

本项目并未完全引入VFS来对各种FS进行抽象，这里所谓`虚拟`，指的是并不存在于物理文件系统内的`虚拟文件和目录`，当用户对其发起读写等操作时将转化为调用内核函数。

## busybox所需虚拟文件系统

- `readlinkat(AT_FDCWD, "/proc/self/exe", "[cmd absolute path]", 4096)`
- `openat(AT_FDCWD, "/proc/[pid]/cmdline", O_RDONLY)`
- `openat(AT_FDCWD, "/proc/[pid]/stat", O_RDONLY)`