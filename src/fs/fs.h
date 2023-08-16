#pragma once

#include <common/types.h>
#include <os/time.h>
// #include <fs/fat32.h>

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

#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif
#define FD_SETIDXMASK (8 * sizeof(unsigned long))
#define FD_SETBITMASK (8 * sizeof(unsigned long) - 1)

#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

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

#define INODE_DIR 1
#define INODE_FILE 0
#pragma pack(2)
typedef struct inode {
    ptr_t i_mapping;  // page cache addr
    uint16_t i_ino;   // inode num
    uint16_t i_upper; // dir inode num
    uint8_t i_type;   // file type 0file 1dir
    uint8_t i_link;   // file link num
    uint32_t i_fclus; // first file cluster
    int i_offset;     // dentry offset in upper dir
    // ptr_t	        padding1;
    // ptr_t	        padding2;
    // ptr_t	        padding3;
    // ptr_t	        padding4;
    ptr_t padding5;
    // uint32_t	    padding6;
    uint16_t padding7;
} inode_t;
#pragma pack()

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
} kstat64_t;

typedef struct fd_set {
    unsigned long fds[(FD_SETSIZE + FD_SETBITMASK) / FD_SETIDXMASK];
} fd_set_t;

typedef struct pollfd {
    int fd;
    short events;
    short revents;
} pollfd_t;

#ifndef __statfs_word
#if __BITS_PER_LONG == 64
#define __statfs_word kernel_long_t
#else
#define __statfs_word __u32
#endif
#endif

typedef struct statfs {
    __statfs_word f_type;
    __statfs_word f_bsize;
    __statfs_word f_blocks;
    __statfs_word f_bfree;
    __statfs_word f_bavail;
    __statfs_word f_files;
    __statfs_word f_ffree;
    kernel_fsid_t f_fsid;
    __statfs_word f_namelen;
    __statfs_word f_frsize;
    __statfs_word f_flags;
    __statfs_word f_spare[4];
} statfs_t;

typedef long long kernel_loff_t;
typedef kernel_loff_t loff_t;

typedef struct iovec {
    void *iov_base;        /* BSD uses caddr_t (1003.1g requires void *) */
    kernel_size_t iov_len; /* Must be size_t (1003.1g) */
} iovec_t;

typedef struct stat {
    unsigned long st_dev;  /* Device.  */
    unsigned long st_ino;  /* File serial number.  */
    unsigned int st_mode;  /* File mode.  */
    unsigned int st_nlink; /* Link count.  */
    unsigned int st_uid;   /* User ID of the file's owner.  */
    unsigned int st_gid;   /* Group ID of the file's group. */
    unsigned long st_rdev; /* Device number, if device.  */
    unsigned long __pad1;
    long st_size;   /* Size of file, in bytes.  */
    int st_blksize; /* Optimal block size for I/O.  */
    int __pad2;
    long st_blocks; /* Number 512-byte blocks allocated. */
    long st_atime;  /* Time of last access.  */
    unsigned long st_atime_nsec;
    long st_mtime; /* Time of last modification.  */
    unsigned long st_mtime_nsec;
    long st_ctime; /* Time of last status change.  */
    unsigned long st_ctime_nsec;
    unsigned int __unused4;
    unsigned int __unused5;
} stat_t;

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

int fs_init();

int fs_load_file(const char *name, uint8_t **bin, int *len);

// [FUNCTION REQUIREMENTS]
bool fs_check_file_existence(const char *name);

// TODO: final syscall
long sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);

long sys_pselect6(int nfds, fd_set_t *readfds, fd_set_t *writefds, fd_set_t *exceptfds, kernel_timespec_t *timeout, void *sigmask);
long sys_ppoll(pollfd_t *fds, unsigned int nfds, kernel_timespec_t *tmo_p, void *sigmask);

long sys_setxattr(const char *path, const char *name, const void *value, size_t size, int flags);
long sys_lsetxattr(const char *path, const char *name, const void *value, size_t, int flags);
long sys_removexattr(const char *path, const char *name);
long sys_lremovexattr(const char *path, const char *name);

long sys_getcwd(char *buf, size_t size);
long sys_dup(int old);
long sys_dup3(int old, int new, mode_t flags);

long sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg);
long sys_flock(unsigned int fd, unsigned int cmd);

long sys_mknodat(int dfd, const char *filename, umode_t mode, unsigned dev);
long sys_mkdirat(int dirfd, const char *path, mode_t mode);

long sys_unlinkat(int dirfd, const char *path, mode_t flags);
long sys_symlinkat(const char *oldname, int newdfd, const char *newname);
long sys_linkat(int old, const char *oldname, int newd, const char *newname, mode_t flags);

long sys_umount2(const char *special, mode_t flags);
long sys_mount(const char *special, const char *dir, const char *type, mode_t flags, void *data);
long sys_pivot_root(const char *new_root, const char *put_old);

long sys_statfs(const char *path, statfs_t *buf);
long sys_ftruncate(unsigned int fd, unsigned long length);
long sys_fallocate(int fd, int mode, loff_t offset, loff_t len);
long sys_faccessat(int dfd, const char *filename, int mode);

long sys_chdir(char *path);
long sys_fchdir(unsigned int fd);
long sys_chroot(const char *filename);
long sys_fchmod(unsigned int fd, umode_t mode);
long sys_fchmodat(int dfd, const char *filename, umode_t mode);
long sys_fchownat(int dfd, const char *filename, uid_t user, gid_t group, int flag);
long sys_fchown(unsigned int fd, uid_t user, gid_t group);

long sys_openat(int dirfd, const char *filename, mode_t flags, mode_t mode);
long sys_close(int fd);
long sys_pipe2(int *fd, mode_t flags);

long sys_getdents64(int fd, dirent64_t *dirent, size_t len);

long sys_lseek(unsigned int fd, off_t offset, unsigned int whence);
ssize_t sys_read(int fd, char *buf, size_t count);
ssize_t sys_write(int fd, const char *buf, size_t count);
long sys_readv(unsigned long fd, const iovec_t *vec, unsigned long vlen);
long sys_writev(unsigned long fd, const iovec_t *vec, unsigned long vlen);

long sys_sendfile64(int out_fd, int in_fd, loff_t *offset, size_t count);
long sys_readlinkat(int dfd, const char *path, char *buf, int bufsiz);

long sys_newfstatat(int dfd, const char *filename, stat_t *statbuf, int flag);
long sys_newfstat(unsigned int fd, stat_t *statbuf);
long sys_fstat64(int fd, kstat64_t *statbuf);
long sys_fstatat64(int fd, const char *filename, kstat64_t *statbuf, int flag);

long sys_sync(void);
long sys_fsync(unsigned int fd);

long sys_umask(int mask);
long sys_syncfs(int fd);
long sys_renameat2(int olddfd, const char *oldname, int newdfd, const char *newname, unsigned int flags);

long sys_munmap(unsigned long addr, size_t len);
long sys_mremap(unsigned long addr, unsigned long old_len, unsigned long new_len, unsigned long flags, unsigned long new_addr);
long sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

long sys_utimensat(int dfd, const char *filename, kernel_timespec_t *utimes, int flags);
long sys_symlink(int dfd, const char *filename);

long k_openat(int dirfd, const char *filename, mode_t flags, mode_t mode);