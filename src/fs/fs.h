#pragma once

#include <common/types.h>

// #define O_RDONLY 0x000
// #define O_WRONLY 0x001
// #define O_RDWR 0x002 // 可读可写
// //#define O_CREATE 0x200
// #define O_CREATE 0x40
// #define O_DIRECTORY 0x0200000

#define DIR 0x040000
#define FILE 0x100000

// #define AT_FDCWD -100

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define STDMAX 3

#define stdin STDIN
#define stdout STDOUT
#define stderr STDERR

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
} dir_entry_t;

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

// extern fentry_t fd[20];
// extern int nowfid;
// extern int freefid[20];
// extern int freenum;

extern int k_mkfs(int func);
int k_load_file(const char *name, uint8_t **bin, int *len);

extern int fat32_init();

extern long sys_getcwd(char *buf, size_t size);
extern int sys_pipe2(int *fd, mode_t flags);
extern int sys_dup(int old);
extern int sys_dup3(int old, int new, mode_t flags);
extern int sys_mkdirat(int dirfd, const char *path, mode_t mode);
extern int sys_chdir(char *path);
extern int sys_getdents64(int fd, dirent64_t *dirent, size_t len);
extern int sys_openat(int dirfd, const char *filename, mode_t flags, mode_t mode);
extern int sys_close(int fd);
extern int sys_linkat(int old, const char *oldname, int newd, const char *newname, mode_t flags);
extern int sys_unlinkat(int dirfd, const char *path, mode_t flags);
extern int sys_mount(const char *special, const char *dir, const char *type, mode_t flags, void *data);
extern int sys_umount2(const char *special, mode_t flags);
extern ssize_t sys_read(int fd, char *buf, size_t count);
extern ssize_t sys_write(int fd, const char *buf, size_t count);
extern int sys_fstat(int fd, kstat_t *statbuf);
void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int sys_munmap(void *addr, size_t length);