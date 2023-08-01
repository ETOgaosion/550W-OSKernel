# 文件系统功能

本小节从不同方面介绍了当前文件系统支持的一些功能以及一些基本实现思路。

## 1.基本磁盘解析函数

包括加载目录/文件，修改目录/文件，文件名解析，遍历目录，切换路径等功能。

文件与目录的加载/写回可以通过目录项中的file size确定大小或者通过first cluster确定其目前占用哪些cluster。

目录的修改主要包括创建子目录/文件等功能。

目录遍历需要通过依次确定目录项的分组、对比名称以及相关信息确定文件在目录中的位置。

路径切换则是根据目标dir数据更新当前dir的信息。

## 2.文件描述符

当前文件描述符在进程之间独立，支持STDIN/STDOUT/STDERR，pipe等必要功能。对于fd(file descriptor)的设计，我们在pcb中加入了一个指向fd的list head，进程初始化时自动创建STDIN、STDOUT、STDERR三个特殊文件描述符，并提供借口进行文件描述符的创建，释放等操作。有一点需要注意的是，clone时文件描述符需要复制一份。

对于STDIN/OUT/ERR类的文件描述符，需要维护输入/输出的缓冲区。

对于pipe，需要维护一个一个ring buffer，实现pipe两端的成功读写。

对于file/dir，需要维护文件的基本信息，文件描述符的访问模式、当前访问位置等数据。

下面展示了一些fd相关接口：

```c
int get_fd_from_list(){
    pcb_t *pcb = *current_running;
    if(list_is_empty(&pcb->fd_head)){
        return STDMAX;
    }
    int fd = STDIN;
    fd_t *file;
    list_for_each_entry(file,&pcb->fd_head,list){
        if(file->fd_num == fd)
            fd++;
        else
            return fd;
    }
    return fd;
}

fd_t *fd_exist(int fd){
    pcb_t *pcb = *current_running;
    if(list_is_empty(&pcb->fd_head)){
        return 0;
    }
    fd_t *file;
    list_for_each_entry(file,&pcb->fd_head,list){
        if(file->fd_num == fd){
            return file;
        }
        if(file->fd_num > fd)
            return NULL;
    }
    return NULL;
}

int fd_alloc(int fd) {
    pcb_t *pcb = *current_running;
    if(fd == -1)
        fd = get_fd_from_list();
    else{
        if(fd_exist(fd)){
            return -1;
        }
    }
    //alloc fd
    fd_t *file = (fd_t *)k_mm_malloc(sizeof(fd_t));
    k_memset(file,0,sizeof(fd_t));
    file->used = 1;
    file->fd_num = fd;
    //add to fd list in pcb
    fd_t *pos;
    list_for_each_entry(pos,&pcb->fd_head,list){
        if(pos->fd_num > fd){
            __list_add(&file->list,pos->list.prev,&pos->list);
            return fd;
        }
    }
    __list_add(&file->list,pcb->fd_head.prev,&pcb->fd_head);
    return fd;
}

int fd_free(int fd) {
    int ret = 0;
    if (fd < STDMAX || fd >= MAX_FD) {
        return -1;
    }
    fd_t *file = fd_exist(fd);
    if(!file)
        return -1;
    __list_del(file->list.prev,file->list.next);
    k_free(file);
    return ret;
}

fd_t *get_fd(int fd) {
    if (fd < STDIN || fd >= MAX_FD) {
        return NULL;
    }
    return fd_exist(fd);
}
```



## 3.系统调用

基于前两部分功能的支持，我们实现了文件系统相关的系统调用，包括：

- **SYS_getcwd 17**

  获取当前工作目录，打印进程维护的当前工作目录

- **SYS_dup 23**

  复制文件描述符，文件引用加1

- **SYS_dup3 24**

  复制到指定的文件描述符，文件引用加1

- **SYS_chdir 49**

  搜索目标目录，切换当前工作目录

- **SYS_openat 56**

  打开或创建文件，并且获取该文件的文件描述符

- **SYS_close 57**

  释放一个文件描述符，文件引用减1

- **SYS_getdents64 61**

  获取文件描述符指向的目录的下一个文件/子目录信息

- **SYS_read 63**

  从文件描述符读区指定长度到buf，返回实际读区的字节数，注意文件末尾

- **SYS_write 64**

  向文件描述符写入buf中的指定长度，返回实际写入内容，注意文件末尾

- **SYS_linkat 37**

  fat32标准不存储文件链接数，可实现在抽象的上层文件系统

- **SYS_unlinkat 35**

  删除指定的文件链接，文件链接数为1时删除

- **SYS_mkdirat 34**

  创建指定的目录

- **SYS_umount2 39**

  取消指定设备的挂载

- **SYS_mount 40**

  将磁盘中的文件系统设备挂载到指定目录下，该目录作为磁盘中文件系统的根目录

- **SYS_fstat 80**

  打印文件描述符指向文件的信息

另外，面向决赛第一阶段的测试，我们还实现了一些额外的系统调用以保证正确性，这里不再一一列举。

## 4.VFS(Virtual File System)实现

我们在文件系统中实现了一个简单的VFS，将系统调用、文件描述符对文件的访问放在VFS之上，提高了文件系统的抽象层次。

VFS的核心数据结构是inode_t，我们在内存中申请了一块连续的空间作为inode table，以数组的形式索引文件的inode。

```c
typedef struct inode {
	  ptr_t	        i_mapping;	//page cache addr
	  uint16_t      i_ino;  		//inode num
	  uint16_t     	i_upper;		//dir inode num
    uint8_t	      i_type;			//file type 0file 1dir
    uint8_t       i_link;			//file link num
    uint32_t      i_fclus;    //first file cluster 
    int           i_offset;		//dentry offset in upper dir
    ptr_t	        padding1;
    uint16_t      padding2;
}inode_t;
```

inode_t中成员变量的设计考虑了文件系统的需求，包括文件在内存中缓存的地址空间，文件的常用基本信息，索引文件对应目录项的条件等等。

文件在第一次被访问时会被映射进内存并在目录条目中标记，之后所有进程对此文件的访问都会定位到映射地址，便于保证文件的一致性和正确性，也简化了文件写回这一过程。例如，fat32文件系统加载时就会将根目录缓存进内存并将根目录信息存储至root_dir，后续对根目录的访问也是从root_dir寻找根目录在内存中的映射。

这里封装了多个函数用作VFS处理的inode时的接口，包括将磁盘文件加入inode table时申请inode的操作，写文件时对文件大小、状态的更新等。

```c
uint16_t alloc_inode(dentry_t *entry,char * name, uint16_t upper, int offset){
  	uint16_t inode_num = get_inodenum();
    if(inode_num >= MAX_INODE){
        return 0;
    }
    inode_table[inode_num].i_ino = inode_num;
    inode_table[inode_num].i_fclus = fat32_dentry2fcluster(entry);
    inode_table[inode_num].i_link = 1;
    inode_table[inode_num].i_mapping = (ptr_t)read_whole_dir(inode_table[inode_num].i_fclus,0);
    inode_table[inode_num].i_type = entry->sn.attr & ATTR_DIRECTORY ? 1 : 0;
    inode_table[inode_num].i_upper = upper;
    inode_table[inode_num].i_offset = offset;
    entry->sn.fst_clus_lo = inode_num;
    entry->sn.nt_res = 1;
    return inode_num++;
}

int update_dentry_size(uint16_t upper,int offset,uint32_t size){
    dentry_t *dtable = (dentry_t *)inode_table[upper].i_mapping;
    if(dtable[offset].sn.attr & ATTR_DIRECTORY)
        dtable[offset].sn.file_size = fat32_fcluster2size(inode_table[upper].i_fclus);
    else
        dtable[offset].sn.file_size = size;
    return 0;
}
```

