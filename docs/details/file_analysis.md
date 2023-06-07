# 文件系统解析

本节主要结合fat32文件系统格式，说明本小组如何实现对fat32磁盘的解析。

## 1.文件系统初始化

对于未分区的fat32磁盘文件来说，0号扇区往往存储了解析磁盘格式所需的全部信息，主要包括：

| Offset(hex) | Size(bytes) | describe                |
| ----------- | ----------- | ----------------------- |
| 0x0B        | 2           | Bytes per sector        |
| 0x0D        | 1           | Sectors per cluster     |
| 0x0E        | 2           | Reserved sectors        |
| 0x10        | 1           | Number of fats          |
| 0x13        | 2           | Sector num(lo)          |
| 0x20        | 4           | Sector num(hi)          |
| 0x1c        | 4           | Hidden sec              |
| 0x024       | 4           | Sectors per fat         |
| 0x02c       | 4           | Cluster num of root dir |
| 0x052       | 8           | Identifier "Fat32     " |

根据这些信息，我们可以进一步计算得到与fat32磁盘交互所需的所有数据，进而完成文件系统的初始化。

## 2.File Allocation Table

Fat32以一个数组形式的File Allocation Table表示磁盘的使用状态。每个文件可以通过保存first cluster来向后寻找到文件占有的所有cluster。

```c
next_cluster = fat[cur_cluster] & FAT_MASK;
```

## 3.目录格式

目录项的大小固定为32B，主要包括文件的权限，大小，名称等信息。为了保存较长的文件名，往往由连续的一个或多个目录项构成一组，共同表示一个文件。每组目录项的最后一个为短目录项，其余为长目录项。

按照fat32规范，如果文件需要长目录项，整个文件名都存储在长目录项中；否则以8.3格式放在短目录中。为避免麻烦，这里可以统一使用长目录项存储文件名。（只要保证自洽）

## 4.磁盘驱动

实现virtio驱动，重点是对磁盘扇区的读写。



[1.]: https://wiki.osdev.org/FAT#BPB_.28BIOS_Parameter_Block.29
[2]: https://www.cnblogs.com/Chary/p/12981056.html