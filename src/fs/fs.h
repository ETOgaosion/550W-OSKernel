#pragma once

#include <common/types.h>

#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR 0x002 // 可读可写
//#define O_CREATE 0x200
#define O_CREATE 0x40
#define O_DIRECTORY 0x0200000

#define DIR 0x040000
#define FILE 0x100000

#define AT_FDCWD -100

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct super_block {
    int magic;
    int fs_start_sec;
    int size;
    int imap_sec_offset;
    int imap_sec_size;
    int smap_sec_offset;
    int smap_sec_size;
    int inode_sec_offset;
    int inode_sec_size;
    int data_sec_offset;
    int data_sec_size;
    int sector_used;
    int inode_used;
    int root_inode_offset;
} super_block_t;

typedef struct dir_entry {
    char name[20];
    int inode_id;
    int last;
    int mode;
} dentry_t;

typedef struct inode_s {
    // pointers need -1
    int sec_size;
    int mode;
    int direct_block_pointers[11];
    int indirect_block_pointers[3];
    int double_block_pointers[2];
    int trible_block_pointers;
    int link_num;
} inode_t;

typedef struct fentry {
    int inodeid;
    int prive;
    int pos_block;
    int pos_offset;
} fentry_t;

typedef struct dirent64 {
	uint64_t d_ino;
	int64_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
} dirent64_t;

typedef struct kstat {
    uint64 st_dev;
    uint64 st_ino;
    mode_t st_mode;
    uint32 st_nlink;
    uint32 st_uid;
    uint32 st_gid;
    uint64 st_rdev;
    unsigned long __pad;
    off_t st_size;
    uint32 st_blksize;
    int __pad2;
    uint64 st_blocks;
    long st_atime_sec;
    long st_atime_nsec;
    long st_mtime_sec;
    long st_mtime_nsec;
    long st_ctime_sec;
    long st_ctime_nsec;
    unsigned __unused[2];
} kstat_t;

extern int fs_start_sec;
extern int magic_number;
extern int sb_sec_offset;
extern int sb2_sec_offset;

extern uint64_t iab_map_addr_offset;
extern uint64_t sec_map_addr_offset;
extern uint64_t iab_map_addr_size;

extern uint64_t inode_addr_offset;
extern uint64_t dir_addr_offset;
extern uint64_t dir2_addr_offset;
extern uint64_t dir3_addr_offset;
extern uint64_t dir4_addr_offset;
extern uint64_t data_addr_offset;
extern uint64_t empty_block;

extern inode_t nowinode;
extern int nowinodeid;

extern fentry_t fd[20];
extern int nowfid;
extern int freefid[20];
extern int freenum;

int try_get_from_file(const char *file_name, unsigned char **binary, int *length);

void init_fs();
int k_mkfs(int func);
long sys_getcwd(const char *buf, unsigned long size);
long sys_dup(unsigned int fildes);
long sys_dup3(unsigned int oldfd, unsigned int newfd, int flags);
long sys_mkdirat(int dfd, const char *pathname, umode_t mode);
long sys_unlinkat(int dfd, const char *pathname, int flag);
long sys_linkat(int olddfd, const char *oldname, int newdfd, const char *newname, int flags);
long sys_umount2(const char *name, int flags);
long sys_mount(const char *dev_name, const char *dir_name, const char *type, unsigned long flags, void *data);
long sys_chdir(const char *file_name);
long sys_openat(int dfd, const char *file_name, int flags, umode_t mode);
long sys_close(unsigned long fd);
long sys_pipe2(int *fildes, int flags);
long sys_getdents64(unsigned int fd, dirent64_t *dirent, unsigned int count);
long sys_read(unsigned int fd, char *buf, size_t count);
long sys_write(unsigned int fd, const char *buf, size_t count);
long sys_fstat(unsigned int fd, kstat_t *statbuf);

void *sys_mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
long sys_munmap(unsigned long addr, size_t len);
long sys_mremap(unsigned long addr,
			   unsigned long old_len, unsigned long new_len,
			   unsigned long flags, unsigned long new_addr);