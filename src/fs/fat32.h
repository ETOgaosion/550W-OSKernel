#pragma once

#include <common/types.h>

#define MAX_FILE_SIZE 0x4000000000

#define FAT32_BOOT_SEC 0

#define BPB_BYTES_PER_SEC 0x0B
#define BPB_SEC_PER_CLUSTER 0x0D
#define BPB_RSVD_SEC_CNT 0x0E
#define BPB_FAT_NUM 0x10
#define BPB_HIDD_SEC 0x1C
#define BPB_TOTL_SEC 0x20
#define BPB_SEC_PER_FAT 0x24
#define BPB_ROOT_CLUSTER 0x2c

typedef struct fat32 {
    struct {
        uint16_t bytes_per_sec;
        uint8_t sec_per_cluster;
        uint16_t rsvd_sec_cnt; /* count of reserved sectors */
        uint8_t fat_num;       /* count of FAT regions */
        uint32_t hidd_sec;     /* count of hidden sectors */
        uint32_t totl_sec;     /* total count of sectors including all regions */
        uint32_t sec_per_fat;  /* count of sectors for a FAT region */
        uint32_t root_cluster; /* cluster num of root dir, typically is 2 */
    } bpb;
    uint32_t first_data_sec; /* root dir sec */
    uint32_t data_sec_cnt;
    uint32_t data_cluster_cnt; /* num of data clusters */
    uint32_t bytes_per_cluster;
    uint32_t entry_per_sec;     /* entry num per fat sec */
    uint32_t free_clusters;     /* num of rest clusters */
    uint32_t next_free_cluster; /* sequentially quick alloc, need a flag(TODO:reuse of hole in SDcard) */
    uint32_t next_free_fat_sec; /* get fat sec quickly */
} fat32_t;

extern fat32_t fat;

#define ATTR_READ_WRITE 0x00
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_FILE_NAME 0x0F
// TODO : for future use?
#define ATTR_LINK 0x40
#define ATTR_CHARACTER_DEVICE 0x80

#define SHORT_FIR_NAME 8
#define SHORT_EXT_NAME 3
#define MAX_SHORT_NAME 11
#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 512 /* TODO : is there any limitation? */
#define DENTRY_LEN 32
// TODO: get_file_name, file name in char* and num increased of next entry
typedef struct short_name_entry {
    char name1[SHORT_FIR_NAME];
    char name2[SHORT_EXT_NAME];
    uint8_t attr;
    uint8_t nt_res;         // 0 not cached, 1 cached
    uint8_t crt_time_tenth; // 39-32 of access
    uint16_t crt_time;      // 31-16 of access
    uint16_t crt_date;      // 15-0 of access
    uint16_t lst_acce_date; // 47-32 of modify
    uint16_t fst_clus_hi;
    uint16_t lst_wrt_time; // 31-16 of modify
    uint16_t lst_wrt_date; // 15-0 of modify
    uint16_t fst_clus_lo;  // inode num if cached
    uint32_t file_size;
} __attribute__((packed, aligned(4))) short_name_entry_t;

#define LONG_NAME1_LEN 5
#define LONG_NAME2_LEN 6
#define LONG_NAME3_LEN 2
#define LONG_NAME_LEN (LONG_NAME1_LEN + LONG_NAME2_LEN + LONG_NAME3_LEN)
#define LAST_LONG_ENTRY 0x40
#define LONG_ENTRY_SEQ 0x1f
typedef struct long_name_entry {
    uint8_t order; /* we used this as the order of next name part, 255 means end*/
    uint16_t name1[LONG_NAME1_LEN];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t name2[LONG_NAME2_LEN];
    uint16_t rsvd;
    uint16_t name3[LONG_NAME3_LEN];
} __attribute__((packed, aligned(4))) long_name_entry_t;

typedef union dentry {
    short_name_entry_t sn;
    long_name_entry_t ln;
} dentry_t;

// typedef struct dir_info {
//     uint32_t first_cluster;
//     uint32_t size;
//     inode_t *node;
//     char name[MAX_PATH_LEN]; // real path
// } dir_info_t;

// extern dir_info_t root_dir, cur_dir;

/* TODO : file descriptor?*/

/*
 * translate cluster to fisrt sec
 */
static inline uint32_t fat_cluster2sec(uint32_t cluster) {
    return ((cluster - 2) * fat.bpb.sec_per_cluster) + fat.first_data_sec;
}

/*
 * translate sec to cluster belonged to
 */
static inline uint32_t fat_sec2cluster(uint32_t sec) {
    return (sec - fat.first_data_sec) / fat.bpb.sec_per_cluster + 2;
}

static inline uint32 fat32_dentry2fcluster(dentry_t *p) {
    return ((uint32_t)p->sn.fst_clus_hi << 16) + p->sn.fst_clus_lo;
}

static inline void fat32_fcluster2dentry(dentry_t *p, uint32 cluster) {
    p->sn.fst_clus_hi = (cluster >> 16) & ((1lu << 16) - 1);
    p->sn.fst_clus_lo = cluster & ((1lu << 16) - 1);
}

#define FAT_MASK 0x0fffffffu
#define FAT_MAX 0x0ffffff8u /* end of the file */
#define FAT_BAD 0x0ffffff7u /* bad cluster of SDcard */
#define FAT_ENTRY_SIZE 4

/*
 * translate cluster to sec and offset of fat1(TODO: no use of fat2)
 */
static inline uint32_t fat_cluster2fatsec(uint32_t cluster) {
    return cluster / fat.entry_per_sec + fat.bpb.hidd_sec + fat.bpb.rsvd_sec_cnt;
}

static inline uint32_t fat_cluster2fatoff(uint32_t cluster) {
    return cluster % fat.entry_per_sec;
}

// static inline uint32_t fat_fcluster2size(uint32_t first){
//     uint32_t ftable[fat.bpb.bytes_per_sec];
//     uint32_t size = 0;
//     uint32_t cur_fat_sec = 0xffffffff;
//     do{
//         if(cur_fat_sec == 0xffffffff || fat_cluster2fatsec(first) != cur_fat_sec){
//             cur_fat_sec = fat_cluster2fatsec(first);
//             // TODO:
//             //fseek(fp,cur_fat_sec*fat.bpb.bytes_per_sec,0);
//             //fread(ftable,fat.bpb.bytes_per_sec,1,fp);

//         }
//         first = ftable[fat_cluster2fatoff(first)] & FAT_MASK;
//         size += fat.bpb.sec_per_cluster;
//     }while(first < FAT_MAX);
//     size = size * fat.bpb.bytes_per_sec;
//     return size;
// }

// utils
static inline char unicode2char(uint16_t unich) {
    return (unich >= 65 && unich <= 90) ? unich - 65 + 'A' : (unich >= 48 && unich <= 57) ? unich - 48 + '0' : (unich >= 97 && unich <= 122) ? unich - 97 + 'a' : (unich == 95) ? '_' : (unich == 46) ? '.' : (unich == 0x20) ? ' ' : (unich == 45) ? '-' : 0;
}
static inline uint16_t char2unicode(char ch) {
    return (ch >= 'A' && ch <= 'Z') ? 65 + ch - 'A' : (ch >= 'a' && ch <= 'z') ? 97 + ch - 'a' : (ch >= '0' && ch <= '9') ? 48 + ch - '0' : (ch == '_') ? 95 : (ch == '.') ? 46 : (ch == ' ') ? 0x20 : (ch == '-') ? 45 : 0;
}