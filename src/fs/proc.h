#pragma once

#include <common/types.h>

char *meminfo = "\
MemTotal:       263777296 kB \n\
MemFree:        260269304 kB \n\
MemAvailable:   260464312 kB \n\
Buffers:          267244 kB  \n\
Cached:          1334704 kB  \n\
SwapCached:            0 kB  \n\
Active:           936436 kB  \n\
Inactive:        1161420 kB  \n\
Active(anon):       2664 kB  \n\
Inactive(anon):   484140 kB  \n\
Active(file):     933772 kB  \n\
Inactive(file):   677280 kB  \n\
Unevictable:           0 kB  \n\
Mlocked:               0 kB  \n\
SwapTotal:             0 kB  \n\
SwapFree:              0 kB  \n\
Dirty:               224 kB  \n\
Writeback:             0 kB  \n\
AnonPages:        496000 kB  \n\
Mapped:           195560 kB  \n\
Shmem:              2684 kB  \n\
KReclaimable:     158064 kB  \n\
Slab:             494356 kB  \n\
SReclaimable:     158064 kB  \n\
SUnreclaim:       336292 kB  \n\
KernelStack:       15536 kB  \n\
PageTables:        14924 kB  \n\
NFS_Unstable:          0 kB  \n\
Bounce:                0 kB  \n\
WritebackTmp:          0 kB  \n\
CommitLimit:    13188864 kB  \n\
Committed_AS:    3497468 kB  \n\
VmallocTotal:   34359738 kB  \n\
VmallocUsed:      249960 kB  \n\
VmallocChunk:          0 kB  \n\
Percpu:            42816 kB  \n\
HardwareCorrupted:     0 kB  \n\
AnonHugePages:         0 kB  \n\
ShmemHugePages:        0 kB  \n\
ShmemPmdMapped:        0 kB  \n\
FileHugePages:         0 kB  \n\
FilePmdMapped:         0 kB  \n\
CmaTotal:              0 kB  \n\
CmaFree:               0 kB  \n\
HugePages_Total:       0     \n\
HugePages_Free:        0     \n\
HugePages_Rsvd:        0     \n\
HugePages_Surp:        0     \n\
Hugepagesize:       2048 kB  \n\
Hugetlb:               0 kB  \n\
DirectMap4k:      483844 kB  \n\
DirectMap2M:     6500352 kB  \n\
DirectMap1G:    263192576 kB";

char *mounts = \
"sysfs /sys sysfs rw,nosuid,nodev,noexec,relatime 0 0\n\
proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0\n\
udev /dev devtmpfs rw,nosuid,noexec,relatime,size=131878952k,nr_inodes=32969738,mode=755 0 0";
