#pragma once

#define O_RDONLY 1 /* read only open */
#define O_WRONLY 2 /* write only open */
#define O_RDWR 3   /* read/write open */

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

int sys_mkfs(int func);
int sys_statfs();
int sys_ls(const char *name, int func);
int sys_rmdir(const char *name);
int sys_mkdir(const char *name);
int sys_cd(const char *name);
int sys_touch(const char *name);
int sys_cat(const char *name);
int sys_fopen(const char *name, int access);
int sys_fread(int fid, char *buff, int size);
int sys_fwrite(int fid, char *buff, int size);
void sys_fclose(int fid);
int sys_ln(const char *name, char *path);
int sys_rm(const char *name);
int sys_lseek(int fid, int offset, int whence);