#include <asm/pgtable.h>
#include <asm/sbi.h>

#include <arch/riscv/include/asm/io.h>
#include <drivers/screen/screen.h>
#include <drivers/virtio/virtio.h>
#include <fs/fat32.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <lib/list.h>
#include <lib/math.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/mm.h>
#include <os/pcb.h>
#include <os/sync.h>
#include <os/sys.h>
#include <os/time.h>
#include <user/user_programs.h>

typedef struct dir_info {
    uint32_t first_cluster;
    uint32_t size;
    inode_t *node;
    char name[MAX_PATH_LEN]; // real path
} dir_info_t;

// fat32 infomation
fat32_t fat;
dir_info_t root_dir = {.name = "/\0"}, cur_dir = {.name = "/\0"};
inode_t *inode_table = NULL;
#define MAX_INODE 1 << 16
uint32_t inode_num = 1;

typedef struct fd_mbox {
    int fd;
    int mailbox;
} fd_mbox_t;

fd_mbox_t fd_mbox_map[MAX_FD];

// /*
//  * return the next cluster of file by current cluster
//  * 0 means file read fail or end of file
//  */
// static inline uint32_t next_cluster(uint32_t cluster){
//     uint32_t* table = k_mm_malloc(fat.bpb.bytes_per_sec);
//     int ret = sbi_sd_read((uint32_t)table,1,fat_cluster2fatsec(cluster));
//     if(ret){
//         k_print("> [FAT32 traverse] fat sector read error!\n");
//         return 0;
//     }
//     uint32_t next = table[fat_cluster2fatoff(cluster)] & FAT_MASK;
//     //kfree(table);

//     if(next >= FAT_MAX)
//         return 0;
//     if(next == FAT_BAD)
//         k_print("> [FAT32 traverse] current cluster %d bad status!\n",cluster);

//     return next;

// }

uint32_t fat32_fcluster2size(uint32_t first) {
    if (first < 2) {
        return 0;
    }
    int size = 0;
    uint32_t ftable[fat.bpb.bytes_per_sec / sizeof(uint32_t)];
    uint32_t cur_fat_sec = 0xffffffff;
    int cnt = 10;
    do {
        if (cur_fat_sec == 0xffffffff || fat_cluster2fatsec(first) != cur_fat_sec) {
            cur_fat_sec = fat_cluster2fatsec(first);
            // TODO:
            // fseek(fp,cur_fat_sec*fat.bpb.bytes_per_sec,0);
            // fread(ftable,fat.bpb.bytes_per_sec,1,fp);
            d_sd_read((char *)ftable, &cur_fat_sec, 1);
            // k_print("read %s \n",buffer[0]->data);
        }
        first = ftable[fat_cluster2fatoff(first)] & FAT_MASK;
        size++;
        cnt--;
        if (!cnt) {
            break;
        }
    } while (first < FAT_MAX && first > 2);
    return size * fat.bytes_per_cluster;
}

// TODO : using when alloc inode, no need to write now
/**
 * @brief  read whle file by first cluster
 * @param  first  id of first cluster
 * @return addr of data, callee should free buffer by calling func(TODO)
 */
void *read_whole_dir(uint32_t first, uint32_t size) {
    if (first < 2) {
        if (size != 0) {
            return k_mm_malloc(((size + fat.bytes_per_cluster - 1) / fat.bytes_per_cluster) * fat.bytes_per_cluster);
        }
        // k_print("[ERROR] read empty file!\n");
        return NULL;
    }
    uint32_t ftable[fat.bpb.bytes_per_sec / sizeof(uint32_t)];
    if (size == 0) {
        size = fat32_fcluster2size(first);
    }
    // k_print("[debug]first %d size = %d, malloc %d sec bytes\n", first, size, (size / fat.bpb.bytes_per_sec + 1) * fat.bpb.bytes_per_sec);
    uint8_t *data = k_mm_malloc(((size + fat.bytes_per_cluster - 1) / fat.bytes_per_cluster) * fat.bytes_per_cluster);
    // k_print("[debug] file size = %u, kalloc %u\n",size,((size + fat.bytes_per_cluster - 1) / fat.bytes_per_cluster) * fat.bytes_per_cluster);
    int cnt = 0;
    uint32_t cur_fat_sec = 0xffffffff;
    do {
        if (cur_fat_sec == 0xffffffff || fat_cluster2fatsec(first) != cur_fat_sec) {
            cur_fat_sec = fat_cluster2fatsec(first);
            // TODO
            //  fseek(fp,cur_fat_sec*fat.bpb.bytes_per_sec,0);
            //  fread(ftable,fat.bpb.bytes_per_sec,1,fp);
            d_sd_read((char *)ftable, &cur_fat_sec, 1);
            // k_print("[debug] read cur sec %x, first %x\n",cur_fat_sec,first);
        }
        // TODO
        // fseek(fp,fat_cluster2sec(first)*fat.bytes_per_cluster,0);
        // fread(&data[cnt*fat.bytes_per_cluster], fat.bytes_per_cluster, 1, fp);

        // read a cluster
        // read_whole_cluster(&data[cnt*fat.bytes_per_cluster],fat.bpb.sec_per_cluster);
        for (int i = 0; i < fat.bpb.sec_per_cluster; i++) {
            uint32_t sec_num = fat_cluster2sec(first) + i;
            // k_print("[debug] read sec %d in cluster %x\n",sec_num,first);
            d_sd_read((char *)(&data[cnt * fat.bytes_per_cluster + i * fat.bpb.bytes_per_sec]), &sec_num, 1);
        }
        first = ftable[fat_cluster2fatoff(first)] & FAT_MASK;
        cnt++;
    } while (first < FAT_MAX && first > 2);

    return (void *)data;
}

uint16_t alloc_inode(dentry_t *entry, char *name, uint16_t upper, int offset) {
    if (inode_num >= MAX_INODE) {
        // k_print("[debug] no rest inode!\n");
        return 0;
    }
    // k_print("[debug] alloc inode, fcluster %d, name %s, upper %d\n",fat32_dentry2fcluster(entry),name,upper);
    inode_table[inode_num].i_ino = inode_num;
    inode_table[inode_num].i_fclus = fat32_dentry2fcluster(entry);
    inode_table[inode_num].i_link = 1;
    inode_table[inode_num].i_mapping = (ptr_t)read_whole_dir(inode_table[inode_num].i_fclus, 0);
    inode_table[inode_num].i_type = entry->sn.attr & ATTR_DIRECTORY ? 1 : 0;
    inode_table[inode_num].i_upper = upper;
    inode_table[inode_num].i_offset = offset;
    entry->sn.fst_clus_lo = inode_num;
    entry->sn.nt_res = 1;
    return inode_num++;
}
/**
 * @brief  write whole file by first cluster
 * @param  first  id of first cluster
 * @param  write  write or not
 * @param  data   data size need to be size from first cluster?
 * @return 0 means success, fail otherwise
 */
int write_whole_dir(uint32_t first, void *data, uint32_t write) {
    if (first < 2 || !write) {
        return 0;
    }
    uint32_t ftable[fat.bpb.bytes_per_sec / sizeof(uint32_t)];
    int cnt = 0;
    uint32_t cur_fat_sec = 0xffffffff;
    do {
        if (cur_fat_sec == 0xffffffff || fat_cluster2fatsec(first) != cur_fat_sec) {
            cur_fat_sec = fat_cluster2fatsec(first);
            // TODO
            // fseek(fp,cur_fat_sec*fat.bpb.bytes_per_sec,0);
            // fread(ftable,fat.bpb.bytes_per_sec,1,fp);
            d_sd_read((char *)ftable, &cur_fat_sec, 1);
        }
        // TODO
        // fseek(fp,fat_cluster2sec(first)*fat.bpb.bytes_per_sec,0);
        // fwrite(&data[cnt*fat.bytes_per_cluster], fat.bytes_per_cluster, 1, fp);
        for (int i = 0; i < fat.bpb.sec_per_cluster; i++) {
            uint32_t sec_num = fat_cluster2sec(first) + i;
            // k_print("[debug] write sec %d in cluster %x\n",sec_num,first);
            d_sd_write((char *)(&((uint8_t *)data)[cnt * fat.bytes_per_cluster + i * fat.bpb.bytes_per_sec]), &sec_num, 1);
        }
        first = ftable[fat_cluster2fatoff(first)] & FAT_MASK;
        cnt++;
    } while (first < FAT_MAX && first > 2);

    // free data
    // kfree(data);
    return 0;
}

int update_dentry_size(uint16_t upper, int offset, uint32_t size) {
    dentry_t *dtable = (dentry_t *)inode_table[upper].i_mapping;
    if (dtable[offset].sn.attr & ATTR_DIRECTORY) {
        dtable[offset].sn.file_size = fat32_fcluster2size(inode_table[upper].i_fclus);
    } else {
        dtable[offset].sn.file_size = size;
    }
    return 0;
}

/**
 * @brief  alloc a nea cluster for a file/dir
 * @param  first  id of first cluster
 * @return alloced cluster num
 */
uint32_t alloc_cluster(uint32_t first) {
    uint32_t cur_fat_sec = 0xffffffff;
    int max = fat.bpb.bytes_per_sec / sizeof(uint32_t);
    uint32_t ftable1[max];
    uint32_t ftable2[max];
    // k_print("[debug] alloc cluster! ftable size = %d\n", sizeof(ftable1));
    // get last cluster num and sec
    if (first != 0) {
        // functionalize first cluster to size?
        uint32_t next;
        do {
            if (cur_fat_sec == 0xffffffff || fat_cluster2fatsec(first) != cur_fat_sec) {
                cur_fat_sec = fat_cluster2fatsec(first);
                // TODO:
                // fseek(fp,cur_fat_sec*fat.bpb.bytes_per_sec,0);
                // fread(ftable1,fat.bpb.bytes_per_sec,1,fp);
                d_sd_read((char *)ftable1, &cur_fat_sec, 1);
            }
            next = ftable1[fat_cluster2fatoff(first)] & FAT_MASK;
            if (next >= FAT_MAX) {
                break;
            }
            first = next;
        } while (1);
    }

    // search empty cluster
    uint32_t i = 0, base = fat_cluster2fatsec(0);
    uint32_t ret = 0;
    while (i < fat.bpb.sec_per_fat) {
        // TODO read sec into ftable
        //  fseek(fp,base*fat.bpb.bytes_per_sec,0);
        //  fread(ftable2,fat.bpb.bytes_per_sec,1,fp);
        d_sd_read((char *)ftable2, &base, 1);
        int j = 0;
        for (j = 0; j < max; j++) {
            // k_print("[debug] fat entry(%d,%d) :%x\n",i,j,ftable2[j] & FAT_MASK);
            if ((ftable2[j] & FAT_MASK) == 0) {
                ftable2[j] = FAT_MAX;
                // k_print("[debug] assigned value %x\n",ftable2[j]);
                // new cluster num
                ret = (i * max + j) & FAT_MASK;
                // TODO : write
                // fseek(fp,(base+i)*fat.bpb.bytes_per_sec,0);
                // k_print("write in %ld bytes\n",fwrite(ftable2,1,fat.bpb.bytes_per_sec,fp));
                // d_sd_write((char *)ftable2, &base, 1);
                // k_print("[debug] alloc cluster %u in sec %u\n", ret, base);
                goto alloced;
            }
        }
        i++;
        base++;
    }

alloced:
    if (first != 0) {
        // reload ftable1 (ftable1 hit same sec of ftable2) and update
        // TODO :
        //  fseek(fp,cur_fat_sec*fat.bpb.bytes_per_sec,0);
        //  fread(ftable1,fat.bpb.bytes_per_sec,1,fp);
        d_sd_read((char *)ftable1, &cur_fat_sec, 1);
        ftable1[fat_cluster2fatoff(first)] = ret;
        // TODO : wtite
        // fseek(fp,cur_fat_sec*fat.bpb.bytes_per_sec,0);
        // fwrite(ftable1,fat.bpb.bytes_per_sec,1,fp);
        // d_sd_write((char *)ftable1, &cur_fat_sec, 1);
        // k_print("[debug] fix tail %u\n", cur_fat_sec);
    }
    return ret;
}

/**
 * @brief divede filename into path and name
 * @param  path     buffer for dir path
 * @param  name     buffer file name
 * @param  filename filename with path
 *
 */
void filename2path(char *path, char *name, const char *filename) {
    int tag = 0, i = 0;
    while (filename[i] != '\0') {
        if (filename[i] == '/') {
            tag = i + 1;
        }
        i++;
    }
    k_memcpy(path, filename, tag);
    path[tag] = '\0';
    i = 0;
    while (filename[tag] != '\0') {
        name[i++] = filename[tag++];
    }
    name[i] = '\0';
}

/**
 * @param  path buffer for dir path
 * @param  name file in now dir
 * @param  len  name len
 *
 */
char *path2name(char *path, char *name) {
    if (path[0] == '/') {
        path++;
    }
    while (path[0] != '/' && path[0] != '\0') {
        name[0] = path[0];
        path++;
        name++;
    }
    name[0] = '\0';
    if (path[0] != '\0') {
        return path;
    }

    return NULL;
}

#define END_OF_DIR 0x00
#define EMPTY_ENTRY 0xE5

int is_end_of_dir(dentry_t *entry) {
    if (entry->sn.name1[0] == 0x00) {
        return 1;
    } else {
        return 0;
    }
}
int is_empty_entry(dentry_t *entry) {
    if ((uint8_t)entry->sn.name1[0] == 0xE5) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  get name from ldentry, return offset of these lentry
 * @param  name  file name
 * @param  entry first entry of LFN entry
 *
 */
int ldentry2name(dentry_t *entry, char *name) {
    long_name_entry_t *long_entry = &entry->ln;
    int ret = 0;
    // 处理长目录项
    if ((long_entry->order & LAST_LONG_ENTRY) == LAST_LONG_ENTRY) {
        uint16_t unicode;
        uint8_t long_entry_num = long_entry->order & LONG_ENTRY_SEQ;
        ret = long_entry_num + 1;
        while (long_entry_num--) {
            int count = 0;
            for (int i = 0; i < LONG_NAME1_LEN; i++) {
                unicode = long_entry->name1[i];
                if (unicode == 0x0000 || unicode == 0xffff) {
                    name[long_entry_num * LONG_NAME_LEN + count] = '\0';
                    break;
                } else {
                    name[long_entry_num * LONG_NAME_LEN + count] = unicode2char(unicode);
                }
                count++;
            }
            for (int i = 0; i < LONG_NAME2_LEN; i++) {
                unicode = long_entry->name2[i];
                if (unicode == 0x0000 || unicode == 0xffff) {
                    name[long_entry_num * LONG_NAME_LEN + count] = '\0';
                    break;
                } else {
                    name[long_entry_num * LONG_NAME_LEN + count] = unicode2char(unicode);
                }
                count++;
            }
            for (int i = 0; i < LONG_NAME3_LEN; i++) {
                unicode = long_entry->name3[i];
                if (unicode == 0x0000 || unicode == 0xffff) {
                    name[long_entry_num * LONG_NAME_LEN + count] = '\0';
                    break;
                } else {
                    name[long_entry_num * LONG_NAME_LEN + count] = unicode2char(unicode);
                }
                count++;
            }
            // entry = next_entry(long_entry, buf, &clus_num, &sec_num);
            // entry = next_entry(long_entry, buf, &nfd->cur_clus_num, &nfd->cur_sec);
            // TODO: now read whole dir at once
            long_entry++;
            // k_print("getdents: %s, pos: %d, cur_clus: %d, cur_sec: %d\n",nfd->name, nfd->pos, nfd->cur_clus_num, nfd->cur_sec);
            // long_entry = (long_name_entry_t *)entry;
        }
    }
    return ret;
}

/**
 * @brief  get name from sdentry, return offset(1) of this entry
 * @param  name  file name
 * @param  entry first entry of file
 *
 */
int sdentry2name(dentry_t *entry, char *name) {
    int count = 0;
    for (int i = 0; i < SHORT_FIR_NAME; i++) {
        if (entry->sn.name1[i] == ' ') {
            break;
        }
        name[count++] = entry->sn.name1[i];
    }

    if (entry->sn.name2[0] == ' ') {
        name[count] = '\0';
        return count;
    }

    name[count++] = '.';
    for (int i = 0; i < SHORT_EXT_NAME; i++) {
        if (entry->sn.name2[i] == ' ') {
            break;
        }
        name[count++] = entry->sn.name2[i];
    }
    name[count] = '\0';
    return 1;
}

/**
 * @brief  get name from dentry, return offset of this whole entry
 * @param  name  file name
 * @param  entry first entry of file
 *
 */
int dentry2name(dentry_t *entry, char *name) {
    if (is_end_of_dir(entry)) {
        return 0;
    }
    if (is_empty_entry(entry)) {
        name[0] = '\0';
        return 1;
    }
    // entry cnt, name len
    int cnt = 0;
    if (entry->sn.attr == ATTR_LONG_FILE_NAME) {
        cnt += ldentry2name(entry, name);
    } else {
        cnt += sdentry2name(entry, name);
    }
    return cnt;
}

// return sentry offset when name match, otherwise return -1
int fat32_name2offset(char *name, dentry_t *entry) {
    int offset = 0;
    while (1) {
        // dentry2name
        int len = 0, res = 1;
        char name1[MAX_NAME_LEN];
        k_memset(name1, 0, MAX_NAME_LEN);
        // 0 means end of dir
        len = dentry2name(&entry[offset], name1);
        if (!len) {
            // not found in dir
            return -1;
        }
        offset += len;
        res = k_strcmp(name, name1);
        if (res) {
            continue;
        }
        return offset - 1;
    }
}

/**
 * @brief  search file in dir, fail then return 0, other means hit
 * @param  name  file name
 * @param  now   now dir info
 *
 */
// reuse when searching file
int fat32_name2dir(char *name, dir_info_t *now) {
    // read each sector, search entries(dentry2name), assign value to now
    int offset = 0;
    dentry_t *dtable = NULL;

    // dtable = (dentry_t *)read_whole_dir(now->first_cluster, 0);
    dtable = (dentry_t *)now->node->i_mapping;

    offset = fat32_name2offset(name, dtable);

    if (offset < 0 || dtable[offset].sn.attr != ATTR_DIRECTORY) {
        // free dtable, nodir with same name
        return 0;
    }

    uint16_t new_node = 0;
    if (!dtable[offset].sn.nt_res) {
        new_node = alloc_inode(&dtable[offset], name, now->node->i_ino, offset);
    } else {
        new_node = dtable[offset].sn.fst_clus_lo;
    }
    now->first_cluster = fat32_dentry2fcluster(&dtable[offset]);
    now->size = dtable[offset].sn.file_size;
    k_memcpy(now->name, name, k_strlen(name) + 1);
    now->node = &inode_table[new_node];
    // get path after return

    return 1;
}

/**
 * @brief  set new dentry
 * @param  name  file name
 * @param  flags file flags
 * @param  len length of LFN
 *
 */
int fat32_new_dentry(dentry_t *entry, uint32_t flags, int len, char *name, int offset) {
    // k_print("[debug] openat add dentry! len = %d, name %s\n",len,name);
    entry[len].sn.attr = (flags & O_DIRECTORY) ? ATTR_DIRECTORY : ATTR_READ_WRITE;
    entry[len].sn.name1[0] = name[0];
    entry[len].sn.name1[1] = ' ';
    entry[len].sn.name2[0] = ' ';
    entry[len].sn.file_size = (flags & O_DIRECTORY) ? fat.bytes_per_cluster : 0;

    fat32_fcluster2dentry(&entry[len], (flags & O_DIRECTORY) ? alloc_cluster(0) : 0);
    int len_0 = len;
    // TODO: dentry create/write time?

    // set name ,all dentry in LFN without ./..
    int i = 0, cnt = 0;
    while (len_0-- > 0) {
        i = len_0;
        for (int j = 0; j < LONG_NAME1_LEN; j++) {
            if (name[cnt] == '\0') {
                entry[i].ln.name1[j] = 0xffff;
                break;
            }
            entry[i].ln.name1[j] = char2unicode(name[cnt++]);
        }
        for (int j = 0; j < LONG_NAME2_LEN; j++) {
            if (name[cnt] == '\0') {
                entry[i].ln.name2[j] = 0xffff;
                break;
            }
            entry[i].ln.name2[j] = char2unicode(name[cnt++]);
        }
        for (int j = 0; j < LONG_NAME3_LEN; j++) {
            if (name[cnt] == '\0') {
                entry[i].ln.name3[j] = 0xffff;
                break;
            }
            entry[i].ln.name3[j] = char2unicode(name[cnt++]);
        }
        entry[i].ln.attr = ATTR_LONG_FILE_NAME;
        entry[i].ln.order = (uint8_t)(len - i);
    }
    entry->ln.order |= LAST_LONG_ENTRY;

    if (!(flags & O_DIRECTORY)) {
        return 0;
    }

    // TODO : alloc inode and set ./.. in dir entry
    uint16_t new_inode = alloc_inode(&entry[len], name, 0, offset);

    // dentry_t *dtable = read_whole_dir(fat32_dentry2fcluster(&entry[len_0]), 0);
    dentry_t *dtable = (dentry_t *)inode_table[new_inode].i_mapping;
    k_bzero(dtable, fat.bytes_per_cluster);
    dtable[0].sn.name1[0] = '.';
    dtable[0].sn.name1[1] = ' ';
    dtable[0].sn.name2[0] = ' ';
    dtable[0].sn.attr = ATTR_DIRECTORY;

    dtable[1].sn.name1[0] = '.';
    dtable[1].sn.name1[1] = '.';
    dtable[1].sn.name1[2] = ' ';
    dtable[1].sn.name2[0] = ' ';
    dtable[1].sn.attr = ATTR_DIRECTORY;

    // return write_whole_dir(inode_table[new_inode].i_ino, dtable, 1);
    return 0;
}

/**
 * @brief  create a new dentry in dtable, return sdentry offest of new entry, -1 means fail
 * @param  dtable  whole table of dir
 * @param  name    file name
 * @param  now     now dir info
 * @param  flags   create file(RW) or dir by O_DIRECTORY
 *
 */
int alloc_dentry(dentry_t **dtable, char *name, mode_t flags, mode_t mode, dir_info_t *now) {
    dentry_t *table = NULL;
    // TODO: get file size by first, there is a better way
    uint32_t size = fat32_fcluster2size(now->first_cluster) / sizeof(dentry_t);
    int len = (k_strlen(name) + LONG_NAME_LEN - 1) / LONG_NAME_LEN;
    uint32_t cnt, i;

    table = *dtable;
    cnt = 0, i = 0;
    while (i < size) {
        if ((uint8_t)table[i].sn.name1[0] == 0xE5) {
            cnt++;
        } else if ((uint8_t)table[i].sn.name1[0] == 0x00) {
            i += len - cnt;
            if (i >= size) {
                break;
            }

            fat32_new_dentry(&table[i - len], flags, len, name, i);
            break;
        } else {
            cnt = 0;
        }
        if (cnt > len) {
            fat32_new_dentry(&table[i - len], flags, len, name, i);
            break;
        }
        i++;
    }
    if (i >= size) {
        // k_print("[debug] openat alloc new cluster!\n");
        alloc_cluster(now->first_cluster);
        update_dentry_size(now->node->i_upper, now->node->i_offset, 0);
        // reload dir table and go to the head of this function?
        // *dtable = read_whole_dir(now->first_cluster, 0);
        now->node->i_mapping = (ptr_t)read_whole_dir(now->first_cluster, 0);
        *dtable = (dentry_t *)now->node->i_mapping;
        // free outside
        table = *dtable;
        // add dentry
        fat32_new_dentry(&table[size], flags, len, name, size + len);
        i = len + size;
    }
    // write_whole_dir(now->first_cluster, *dtable, 1);
    return i;
}

/**
 * @brief  destroy an dir entry, include LFN and sn
 * @param  dtable  whole table of dir
 * @param  offset  offset for sn
 */
int destroy_dentry(dentry_t *dtable, int offset) {
    int i = offset - 1;
    while (dtable[i].ln.attr == ATTR_LONG_FILE_NAME) {
        dtable[i].ln.order = 0xE5;
        k_bzero(dtable[i].ln.name1, sizeof(dentry_t) - 1);
        i--;
    }
    k_bzero(dtable[offset].sn.name1, sizeof(dentry_t));
    dtable[offset].sn.name1[0] = 0xE5;
    return 0;
}

/**
 * @brief  fail return 0
 * @param  path buffer for dir path
 * @param  new  target dir info
 * @param  now  now dir info
 *
 */
int fat32_path2dir(char *path, dir_info_t *new, dir_info_t now) {
    char name[MAX_NAME_LEN];
    int ret = 0;
    // search sequentially
    do {
        path = path2name(path, name);
        if (name[0] == '\0') { // chdir to now itserf
            ret = 1;
            break;
        }
        if (!k_strcmp(".", name)) {
            ret = 1;
        } else {
            ret = fat32_name2dir(name, &now);
        }
    } while (path != NULL && ret != 0);

    // not found return NULL, found return dir_info to update cwd
    if (!ret) {
        return 0;
    } else {
        new->first_cluster = now.first_cluster;
        new->size = now.size;
        k_strcpy(new->name, now.name);
        new->node = now.node;
        return 1;
    }
}

int fat32_dentry2fd(dentry_t *entry, fd_t *file, mode_t flags, mode_t mode, char *name) {
    if (entry->sn.attr & ATTR_DIRECTORY) {
        file->file = ATTR_DIRECTORY;
    } else {
        file->file = ATTR_DIRECTORY >> 1;
    }
    k_memcpy(file->name, name, k_strlen(name) + 1);
    file->dev = 0; // TODO
    file->flags = flags & ~(O_DIRECTORY);
    file->mode = mode;
    file->first_cluster = fat32_dentry2fcluster(entry);
    file->pos = 0;
    file->size = entry->sn.file_size;
    return 0;
}

int fs_init() {
    k_print("> [FAT32 init] begin!\n");

    fd_table_init();

    // read BOOT_SEC
    uint8_t *boot = k_mm_malloc(PAGE_SIZE);
    uint32_t boot_sec = 0;

    d_sd_read((char *)boot, &boot_sec, 1);
    // TODO:check fat32? should not be check device name?

    // little endian not need do add?
    fat.bpb.bytes_per_sec = *(uint16_t *)(boot + BPB_BYTES_PER_SEC);
    fat.bpb.sec_per_cluster = *(boot + BPB_SEC_PER_CLUSTER);
    fat.bpb.rsvd_sec_cnt = *(uint16_t *)(boot + BPB_RSVD_SEC_CNT);
    fat.bpb.fat_num = *(boot + BPB_FAT_NUM);
    fat.bpb.hidd_sec = *(uint32_t *)(boot + BPB_HIDD_SEC);
    fat.bpb.totl_sec = *(uint32_t *)(boot + BPB_TOTL_SEC);
    fat.bpb.sec_per_fat = *(uint32_t *)(boot + BPB_SEC_PER_FAT);
    fat.bpb.root_cluster = *(uint32_t *)(boot + BPB_ROOT_CLUSTER);

    fat.bytes_per_cluster = (uint32_t)fat.bpb.bytes_per_sec * (uint32_t)fat.bpb.sec_per_cluster;
    fat.first_data_sec = (uint32_t)fat.bpb.hidd_sec + (uint32_t)fat.bpb.rsvd_sec_cnt + (uint32_t)fat.bpb.fat_num * fat.bpb.sec_per_fat;
    fat.data_sec_cnt = fat.bpb.totl_sec - fat.first_data_sec;
    fat.data_cluster_cnt = fat.data_sec_cnt / (uint32_t)fat.bpb.sec_per_cluster;
    fat.next_free_fat_sec = (uint32_t)fat.bpb.hidd_sec + (uint32_t)fat.bpb.rsvd_sec_cnt;
    fat.entry_per_sec = (uint32_t)fat.bpb.bytes_per_sec >> 2;
    // TODO: root dir use one cluster?
    fat.free_clusters = fat.data_cluster_cnt - 1;
    fat.next_free_cluster = fat.bpb.root_cluster + 1;

    k_print("> [FAT32 init] byts_per_sec: %d\n", fat.bpb.bytes_per_sec);
    k_print("> [FAT32 init] root_clus: %d\n", fat.bpb.root_cluster);
    k_print("> [FAT32 init] sec_per_clus: %d\n", fat.bpb.sec_per_cluster);
    k_print("> [FAT32 init] fat_cnt: %d\n", fat.bpb.fat_num);
    k_print("> [FAT32 init] fat_sz: %d\n", fat.bpb.sec_per_fat);
    k_print("> [FAT32 init] fat_rsvd: %d\n", fat.bpb.rsvd_sec_cnt);
    k_print("> [FAT32 init] fat_hid: %d\n", fat.bpb.hidd_sec);
    k_print("> [FAT32 init] fat_totl: %d\n", fat.bpb.totl_sec);
    k_print("> [FAT32 init] fat_byts_per_clus: %d\n", fat.bytes_per_cluster);
    k_print("> [FAT32 init] fat_first_data_sec: %d\n", fat.first_data_sec);
    k_print("> [FAT32 init] fat_data_cluster_cnt: %d\n", fat.data_cluster_cnt);
    k_print("> [FAT32 init] fat.next_free_cluster: %d\n", fat.next_free_cluster);

    // TODO: change into inode way
    //  k_print("[debug] size of inode %d\n",sizeof(inode_t));
    inode_table = (inode_t *)k_mm_alloc_page(2048); // total 2^16 inodes
    inode_table[0].i_fclus = fat.bpb.root_cluster;
    inode_table[0].i_ino = 0;
    inode_table[0].i_link = 1;
    inode_table[0].i_mapping = (ptr_t)read_whole_dir(inode_table[0].i_fclus, 0);
    inode_table[0].i_type = INODE_DIR;
    inode_table[0].i_upper = 0; // root is from root

    // set root dir and cwd
    root_dir.first_cluster = fat.bpb.root_cluster;
    root_dir.size = fat32_fcluster2size(root_dir.first_cluster);
    root_dir.node = &inode_table[0];
    root_dir.name[0] = '/';
    root_dir.name[1] = '\0';
    k_print("> [FAT32 INIT] root dir begin at cluster %d, size = %d\n", root_dir.first_cluster, fat.bytes_per_cluster);
    cur_dir.first_cluster = root_dir.first_cluster;
    cur_dir.size = root_dir.size;
    cur_dir.node = &inode_table[0];
    cur_dir.name[0] = '/';
    cur_dir.name[1] = '\0';
    // kfree(boot);

    // dentry_t *dtable = (dentry_t *)inode_table[0].i_mapping;
    // char name[MAX_NAME_LEN];
    // int offset = 0;
    // offset += dentry2name(&dtable[offset], name);
    // k_print("[debug] name1 = %s!, is DIR : %d, has inode : %d\n",name,dtable[offset-1].sn.attr & ATTR_DIRECTORY,dtable[offset-1].sn.nt_res);

    // offset = dentry2name(&dtable[offset], name);
    // k_print("[debug] name2 = %s!\n",name);

    // //[read] ok
    // int read_fd = sys_openat(AT_FDCWD,"./text.txt",O_RDONLY,0);
    // fd_t *file_txt = get_fd(read_fd);
    // if(file_txt)
    //     k_print("[test] 1. read openat success! fd %d, name %s\n",read_fd, file_txt->name);
    // int read_len = sys_read(read_fd,name,256);
    // // k_print("[test] 2. read %d bytes : %s\n",read_len,name);
    // sys_write(STDOUT,name,read_len);
    // fd_free(read_fd);

    // //[chdir] ok(cwd is path or name?)
    // sys_mkdirat(AT_FDCWD,"test_chdir", 0);
    // sys_chdir("test_chdir");
    // sys_getcwd(name,MAX_NAME_LEN);
    // k_print("[test] getcwd %s\n",name);

    // // [dup] ok
    // int dup_fd = sys_dup(STDOUT);
    // k_print("[test] dup fd %d\n",dup_fd);
    // sys_write(dup_fd,"[test] dup!\n",12);

    // // [dup] ok
    // sys_dup3(STDOUT,100,0);
    // const char *str = "  from fd 100\n";
    // sys_write(100, str, k_strlen(str));

    // // [fstats] ok
    // long sys_fd = sys_openat(AT_FDCWD,"./text.txt",O_RDONLY,0);
    // kstat64_t kst;
    // sys_fstat64(sys_fd, &kst);
    // k_print("fstat: dev: %d, inode: %d, mode: %d, nlink: %d, size: %d, atime: %d, mtime: %d, ctime: %d\n",
    //       kst.st_dev, kst.st_ino, kst.st_mode, kst.st_nlink, kst.st_size, kst.st_atime_sec, kst.st_mtime_sec, kst.st_ctime_sec);

    // // [getcwd] ok

    // // [get dentry] ok
    // dirent64_t *test_getdentry = k_mm_malloc(512);
    // int fd = sys_openat(AT_FDCWD,".",O_RDONLY,0);
    // int cnt = 0;
    // while(sys_getdents64(fd,test_getdentry,512)){
    //     k_print("[test] get %d dentry name %s, attr %d\n", cnt++, test_getdentry->d_name, test_getdentry->d_type & ATTR_DIRECTORY);
    // }

    // // [mkdir at]
    // int mkdir_ret = sys_mkdirat(AT_FDCWD,"test_mkdir",0);
    // int mkdir_fd = sys_openat(AT_FDCWD,"test_mkdir", O_RDONLY | O_DIRECTORY,0);
    // k_print("mkdir ret: %d openat fd %d\n", mkdir_ret,mkdir_fd);

    // // [mount] ok
    // char mntpoint[64] = "./mnt";
    // char device[64] = "/dev/vda2";
    // const char *fs_type = "vfat";
    // k_print("Mounting dev:%s to %s\n", device, mntpoint);
    // int ret = sys_mount(device, mntpoint, fs_type, 0, NULL);
    // k_print("mount return: %d\n", ret);
    // if (ret == 0) {
    // 	k_print("mount successfully\n");
    // 	ret = sys_umount2(mntpoint,0);
    // 	k_print("umount return: %d\n", ret);
    // }

    // // [unlink] ok
    // char *fname = "./test_unlink";
    // int unlink_fd, unlink_ret;

    // unlink_fd = sys_openat(AT_FDCWD,fname, O_CREATE | O_WRONLY,0);
    // sys_close(unlink_fd);
    // unlink_ret = sys_unlinkat(AT_FDCWD,fname,0);
    // unlink_fd = sys_openat(AT_FDCWD,fname, O_RDONLY,0);
    // if(unlink_fd < 0 && unlink_ret == 0){
    //     k_print("  unlink success!\n");
    // }else{
    //     k_print("  unlink error!\n");
    //     sys_close(unlink_fd);
    // }

    // // [write] ok
    // const char *wstr = "Hello operating system contest.\n";
    // int str_len = k_strlen(wstr);
    // if(str_len == sys_write(STDOUT, wstr, str_len)){
    //     k_print("[test] write success!\n");
    // }else{
    //     k_print("[test] write fail? strlen = %d, write len = %d",str_len,sys_write(STDOUT, wstr, str_len));
    // }

    // [pipe]

    // uint8_t* disk_data;
    // uint8_t* elf_data;
    // int disk_len;
    // int elf_len;
    // int ret = fs_load_file("test_virtio.elf",&disk_data,&disk_len);
    // if(!ret)
    //     ret = get_elf_file("virtio",&elf_data,&elf_len);
    // else
    //     ret = 0;
    // if(disk_len==elf_len && ret !=0){
    //     k_print("[debug] get same shell.elf length %d!\n",disk_len);
    //     int len = disk_len;
    //     while(len-- > 0){
    //         if(elf_data[len]!=disk_data[len]){
    //             k_print("[debug] file not same at offset %d, elf %u, disk %u\n",len,elf_data[len],disk_data[len]);
    //             break;
    //         }
    //     }
    //     k_print("[debug] cmp finish!\n");
    // }else{
    //     k_print("[debug] get diff shell.elf length! elf len %d, disk len %d\n",elf_len,disk_len);
    // }
    // kfree
    return 0;
}

/**
 * @brief  load elf file in root dir, 0 success, -1 fail
 * @param  name  elf file should be loaded
 * @return elf bit array
 */
// TODO if inode alloced dentry has been changed?
int fs_load_file(const char *name, uint8_t **bin, int *len) {
    dentry_t *dtable = NULL;
    // TODO: read in dir data
    dtable = (dentry_t *)inode_table[0].i_mapping;
    int offset = fat32_name2offset((char *)name, dtable);
    if (offset < 0) {
        // k_print("[debug] can't load %s, no such file!\n",name);
        return -1;
    }
    uint32_t first = fat32_dentry2fcluster(&dtable[offset]);
    // free buffer of dtable
    *len = dtable[offset].sn.file_size;
    if (*len == 0) {
        // k_print("[debug] can't load, empty file!\n");
        return -1;
    }
    // TODO alloc inode for file and cache it?
    *bin = (uint8_t *)read_whole_dir(first, 0);
    return 0;
}

// [FUNCTION REQUIREMENTS]
bool fs_check_file_existence(const char *filename) {
    // get dir file belonged to
    dir_info_t new;
    char path[MAX_PATH_LEN], name[MAX_NAME_LEN];
    int ret = 0;
    filename2path(path, name, filename);
    ret = fat32_path2dir(path, &new, root_dir);
    // not found by path
    if (!ret) {
        return false;
    }
    if (!k_strcmp(".", name)) {
        return true;
    }
    int offset = 0;
    dentry_t *dtable = NULL;
    dtable = (dentry_t *)new.node->i_mapping;
    offset = fat32_name2offset(name, dtable);
    if (offset >= 0) {
        return true;
    } else {
        return false;
    }
}

#define _IOC_NRBITS 8
#define _IOC_TYPEBITS 8
#define _IOC_SIZEBITS 13
#define _IOC_DIRBITS 3

#define _IOC_NRMASK ((1 << _IOC_NRBITS) - 1)
#define _IOC_TYPEMASK ((1 << _IOC_TYPEBITS) - 1)
#define _IOC_SIZEMASK ((1 << _IOC_SIZEBITS) - 1)
#define _IOC_DIRMASK ((1 << _IOC_DIRBITS) - 1)

#define _IOC_NRSHIFT 0
#define _IOC_TYPESHIFT (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_SIZESHIFT (_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT (_IOC_SIZESHIFT + _IOC_SIZEBITS)

/*
 * Direction bits _IOC_NONE could be 0, but OSF/1 gives it a bit.
 * And this turns out useful to catch old ioctl numbers in header
 * files for us.
 */
#define _IOC_NONE 1U
#define _IOC_READ 2U
#define _IOC_WRITE 4U
/**
 * @brief  ioctl load/write cmd to fd
 * @param  name  elf file should be loaded
 * @return elf bit array
 */
long sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg) {
    if (fd >= MAX_FD) {
        return -1;
    }
    uint32_t dir = cmd >> (_IOC_NRBITS + _IOC_TYPEBITS + _IOC_SIZEBITS);
    uint32_t size = (cmd & _IOC_SIZEMASK) >> (_IOC_NRBITS + _IOC_TYPEBITS);
    // no use of type(name of dev)/nr(num of ioctl)
    if (dir == _IOC_NONE) {
        return 0;
    } else if (dir == _IOC_READ && fd == stdin) {
        // need read?
        return 0;
    } else if (dir == _IOC_WRITE && fd == stdout) {
        sys_write(fd, (const char *)arg, size);
        return 0;
    } else if (dir == _IOC_WRITE && fd == stdout) {
        // stderr?
        return 0;
    }
    return 0;
}

long sys_pselect6(int nfds, fd_set_t *readfds, fd_set_t *writefds, fd_set_t *exceptfds, kernel_timespec_t *timeout, void *sigmask) {
    return 0;
}

long sys_ppoll(pollfd_t *fds, unsigned int nfds, kernel_timespec_t *tmo_p, void *sigmask) {
    return 0;
}

long sys_setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
    return 0;
}

long sys_lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
    return 0;
}

long sys_removexattr(const char *path, const char *name) {
    return 0;
}

long sys_lremovexattr(const char *path, const char *name) {
    return 0;
}

/**
 * @param  buf buffer for cur_dir path
 * @param  size bytes of buf
 *
 */
long sys_getcwd(char *buf, size_t size) {
    k_memcpy(buf, cur_dir.name, min(k_strlen(cur_dir.name) + 1, size));
    return (long)buf;
}

/**
 * @brief  copy a fd and return the new one
 * @param  old old fd
 *
 */
long sys_dup(int old) {
    fd_t *file1 = get_fd(old);
    if (!file1 && old != STDIN && old != STDOUT && old != STDERR) {
        return -1;
    }
    int new = fd_alloc(-1);
    if (new < 0) {
        return -1;
    }
    fd_t *file2 = get_fd(new);
    if (!file2) {
        return -1;
    }
    if (old >= STDIN && old <= STDERR) {
        file2->file = old;
    } else {
        list_head list;
        k_memcpy(&list, &file2->list, sizeof(list_head));
        k_memcpy(file2, file1, sizeof(fd_t));
        k_memcpy(&file2->list, &list, sizeof(list_head));
    }
    file2->fd_num = new;
    return new; // fail return -1
}

/**
 * @brief  copy a fd and return the new one
 * @param  old old fd
 * @param  new new fd
 */
long sys_dup3(int old, int new, mode_t flags) {
    fd_t *file1 = get_fd(old);
    if (!file1 && old != STDIN && old != STDOUT && old != STDERR) {
        return -1;
    }
    if (!fd_exist(new)) {
        if (new != fd_alloc(new)) {
            // k_print("[debug] fd %d alloc fail!\n",new);
            return -1;
        }
    }
    fd_t *file2 = get_fd(new);
    if (file1->file >= STDIN && file1->file <= STDERR) {
        file2->file = file1->file;
    } else {
        list_head list;
        k_memcpy(&list, &file2->list, sizeof(list_head));
        k_memcpy(file2, file1, sizeof(fd_t));
        k_memcpy(&file2->list, &list, sizeof(list_head));
        // file2->file = file1->file;
        // file2->first_cluster = file1->first_cluster;
        // file2->dev = file1->dev;
        // file2->flags = file1->flags;
        // file2->inode = file1->inode;
        // file2->pos = file1->pos;
        // file2->used = file1->used;
        // file2->mode = file1->mode;
        // file2->nlink = file1->nlink;
        // file2->size = file1->size;
        // k_strcpy(file2->name,file1->name);
    }
    file2->fd_num = new;
    return new; // fail return -1
}

#define F_DUPFD 0 /* dup */
#define F_GETFD 1 /* get close_on_exec */
#define F_SETFD 2 /* set/clear close_on_exec */
#define F_GETFL 3 /* get file->f_flags */
#define F_SETFL 4 /* set file->f_flags */
#define F_DUPFD_CLOEXEC 1030
long sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg) {
    fd_t *file = get_fd(fd);
    if (!file) {
        return -1;
    }
    if (cmd == F_DUPFD_CLOEXEC) {
        sys_dup3(fd, arg, 0);
        return arg;
    } else if (cmd == F_GETFD) {
        return file->mode;
    } else if (cmd == F_SETFD) {
        file->mode = (uint32_t)arg;
        return 0;
    } else if (cmd == F_GETFL) {
        return file->flags;
    } else if (cmd == F_SETFL) {
        file->flags = (uint32_t)arg;
        return 0;
    }
    // no more cmd!
    return 0;
}

long sys_flock(unsigned int fd, unsigned int cmd) {
    return 0;
}

long sys_mknodat(int dfd, const char *filename, umode_t mode, unsigned dev) {
    return 0;
}

/**
 * @brief  mk dir at patch, success 0 fail -1
 * @param  dir_fd mkdir at this dir
 * @param  flag   AT_FDCWD means mkdir at cwd
 * @param  path   path if from '/', other just means dir name
 */
long sys_mkdirat(int dirfd, const char *path_0, mode_t mode) {
    // get dir file belonged to
    dir_info_t new;
    char path[MAX_PATH_LEN], name[MAX_NAME_LEN];
    int ret = 0;
    int use_fd = 0;
    filename2path(path, name, path_0);
    // k_print("[debug] mkdir %s, at %s\n", name, path);
    if (path[0] == '/') {
        ret = fat32_path2dir(path, &new, root_dir);
    } else if (dirfd == AT_FDCWD) {
        ret = fat32_path2dir(path, &new, cur_dir);
    } else {
        fd_t *file = get_fd(dirfd);
        if (!file || file->file != ATTR_DIRECTORY) {
            return -1;
        }
        use_fd = 1;
        new.first_cluster = file->first_cluster;
        new.node = (inode_t *)file->inode;
        new.size = file->size;
        ret = 1;
    }

    // not found by path
    if (!ret) {
        return -1;
    }

    if (!k_strcmp(".", name)) {
        return -1;
    }
    int offset = 0;
    dentry_t *dtable = NULL;
    // TODO
    //  dtable = read_whole_dir(new.first_cluster, 0);
    dtable = (dentry_t *)new.node->i_mapping;
    offset = fat32_name2offset(name, dtable);

    if (offset >= 0) {
        return -1;
    }

    // k_print("[fat32] openat create file with name %s!\n",name);
    offset = alloc_dentry(&dtable, name, O_DIRECTORY, mode, &new);
    alloc_inode(&dtable[offset], name, new.node->i_ino, offset);
    if (use_fd) {
        fd_t *file = get_fd(dirfd);
        file->size = fat32_fcluster2size(file->first_cluster);
    }
    return 0;
}

/**
 * @brief  rm file directed by path, 0 success -1 fail
 * @param  dirfd     file descripter
 * @param  path      dile path
 * @param  count     0 or AT_REMOVEDIR
 */
long sys_unlinkat(int dirfd, const char *path_0, mode_t flags) {
    // get dir file belonged to
    dir_info_t new;
    char path[MAX_PATH_LEN], name[MAX_NAME_LEN];
    int ret = 0;
    filename2path(path, name, path_0);
    // k_print("[debug] rm %s, at %s\n",name,path);
    if (path[0] == '/') {
        ret = fat32_path2dir(path, &new, root_dir);
    } else if (dirfd == AT_FDCWD) {
        ret = fat32_path2dir(path, &new, cur_dir);
    } else {
        fd_t *file = get_fd(dirfd);
        if (!file || file->file != ATTR_DIRECTORY) {
            return -1;
        }
        new.first_cluster = file->first_cluster;
        new.node = (inode_t *)file->inode;
        new.size = file->size;
        ret = 1;
    }

    // not found by path
    if (!ret) {
        return -1;
    }

    // try open file
    int offset = 0;
    dentry_t *dtable = NULL;
    // TODO
    //  dtable = read_whole_dir(new.first_cluster, 0);
    dtable = (dentry_t *)new.node->i_mapping;

    offset = fat32_name2offset(name, dtable);
    if (offset < 0) {
        // k_print("[debug] : testopen not found!\n");
        // free dtable
        return -1;
    }
    if (!(((dtable[offset].sn.attr & ATTR_DIRECTORY) == 0) ^ ((flags & AT_REMOVEDIR) == 0))) {
        // k_print("[debug] destroy %s at dir %s, offset %d\n",name,path,offset);
        // TODO: use inode ?
        destroy_dentry(dtable, offset);
        // write_whole_dir(new.first_cluster, dtable, 1);
        return 0;
    }
    return -1;
}

long sys_symlinkat(const char *oldname, int newdfd, const char *newname) {
    return 0;
}

long sys_linkat(int old, const char *oldname, int newd, const char *newname, mode_t flags) {
    return 0; // fat not support it! just return 0
    // //get dir file belonged to
    // dir_info_t new;
    // char path[MAX_PATH_LEN], name[MAX_NAME_LEN];
    // int ret = 0;
    // filename2path(path,name,oldname);
    // k_print("open %s, at %s\n",name,path);
    // if(path[0] == '/')
    //     ret = fat32_path2dir(path,&new,root_dir);
    // else if(old == AT_FDCWD)
    //     ret = fat32_path2dir(path,&new,cur_dir);
    // else{
    //     fd_t *file = get_fd(old);
    //     if(!file || file->file != ATTR_DIRECTORY)
    //         return -1;
    //     new.first_cluster = file->first_cluster;
    //     new.size      = fat32_fcluster2size(new.first_cluster);
    //     //no use of name
    // }

    // //not found by path
    // if(!ret)
    //     return -1;

    // //try open file
    // int offset = 0;
    // dentry_t* dtable = NULL;

    // dtable = read_whole_dir(new.first_cluster,0);

    // offset = fat32_name2offset(name,dtable);

    // if(offset >= 0){
    //     if(!(dtable[offset].sn.attr & ATTR_DIRECTORY)){
    //         return -1;
    //     }
    //     dtable[offset].sn.
    // }else{
    //     return -1;
    // }

    // fd_free(ret);
    // return -1;
}

long sys_umount2(const char *special, mode_t flags) {
    // todo
    return 0;
}

long sys_mount(const char *special, const char *dir, const char *type, mode_t flags, void *data) {
    // todo
    return 0;
}

long sys_pivot_root(const char *new_root, const char *put_old) {
    return 0;
}

long sys_statfs(const char *path, statfs_t *buf) {
    return 0;
}

long sys_ftruncate(unsigned int fd, unsigned long length) {
    return 0;
}

long sys_fallocate(int fd, int mode, loff_t offset, loff_t len) {
    return 0;
}

long sys_faccessat(int dfd, const char *filename, int mode) {
    // get dir file belonged to
    dir_info_t new;
    char path[MAX_PATH_LEN], name[MAX_NAME_LEN];
    int ret = 0;
    filename2path(path, name, filename);
    // k_print("[debug] open %s, at %s\n",name,path);
    if (path[0] == '/') {
        ret = fat32_path2dir(path, &new, root_dir);
    } else if (dfd == AT_FDCWD) {
        ret = fat32_path2dir(path, &new, cur_dir);
    } else {
        fd_t *file = get_fd(dfd);
        if (!file || file->file != ATTR_DIRECTORY) {
            return -1;
        }
        new.first_cluster = file->first_cluster;
        new.node = (inode_t *)file->inode;
        new.size = file->size;
        ret = 1;
    }
    // not found by path
    if (!ret) {
        return -1;
    }

    // k_print("[debug] fd %d, name = %s\n",ret,name);
    if (!k_strcmp(".", name)) {
        new.node = &inode_table[new.node->i_upper];
        new.first_cluster = new.node->i_fclus;
        k_strcpy(new.name, name);
        new.size = fat32_fcluster2size(new.first_cluster);
    }
    int offset = 0;
    dentry_t *dtable = NULL;
    // dtable = read_whole_dir(new.first_cluster, 0);
    dtable = (dentry_t *)new.node->i_mapping;
    offset = fat32_name2offset(name, dtable);
    // k_print("[debug] hit dir inode %d, offset %d, cond %d\n",new.node->i_ino,offset,((dtable[offset].sn.attr & ATTR_DIRECTORY) == 0) == ((flags & O_DIRECTORY) == 0));
    uint8_t attr = (mode == O_WRONLY || mode == O_RDWR) && (dtable[offset].sn.attr == ATTR_READ_ONLY);
    if (offset >= 0 && !attr) {
        return 0;
    }
    return -1;
}

/**
 * @brief  change dir by patch, fail return -1
 * @param  path buffer for dir path
 *
 */
long sys_chdir(char *path) {
    dir_info_t new;
    int ret = 0;
    if (path[0] == '/') {
        ret = fat32_path2dir(path, &new, root_dir);
    } else {
        ret = fat32_path2dir(path, &new, cur_dir);
    }

    if (!ret) {
        k_print("No such Directory!\n");
        return -1;
    }
    cur_dir.first_cluster = new.first_cluster;
    cur_dir.size = new.size;
    // path from cur dir or root dir?
    if (path[0] == '/') {
        k_memcpy(cur_dir.name, path, k_strlen(path) + 1);
    } else {
        // TODO : fix '/' error
        k_memcpy((&cur_dir.name[k_strlen(cur_dir.name)]), path, k_strlen(path) + 1);
    }
    return 0;
}

long sys_fchdir(unsigned int fd) {
    return 0;
}

long sys_chroot(const char *filename) {
    return 0;
}

long sys_fchmod(unsigned int fd, umode_t mode) {
    return 0;
}

long sys_fchmodat(int dfd, const char *filename, umode_t mode) {
    return 0;
}

long sys_fchownat(int dfd, const char *filename, uid_t user, gid_t group, int flag) {
    return 0;
}

long sys_fchown(unsigned int fd, uid_t user, gid_t group) {
    return 0;
}

/**
 * @brief  open a file with path/cwd+path/dirfd+path
 * @param  fd     file descripter
 * @param  buf    bytes buf
 * @param  count  read len
 */
long sys_openat(int dirfd, const char *filename, mode_t flags, mode_t mode) {
    // get dir file belonged to'
    dir_info_t new;
    char path[MAX_PATH_LEN], name[MAX_NAME_LEN];
    int ret = 0;
    int use_fd = 0;
    filename2path(path, name, filename);
    // k_print("[debug] open %s, at %s\n",name,path);
    if (path[0] == '/') {
        ret = fat32_path2dir(path, &new, root_dir);
    } else if (dirfd == AT_FDCWD) {
        ret = fat32_path2dir(path, &new, cur_dir);
    } else {
        fd_t *file = get_fd(dirfd);
        if (!file || file->file != ATTR_DIRECTORY) {
            return -1;
        }
        use_fd = 1;
        new.first_cluster = file->first_cluster;
        new.node = (inode_t *)file->inode;
        new.size = file->size;
        ret = 1;
    }
    // not found by path
    if (!ret) {
        return -1;
    }
    // try open file
    ret = fd_alloc(-1);
    fd_t *file = get_fd(ret);
    // k_print("[debug] fd %d, name = %s\n",ret,name);
    if (!k_strcmp(".", name)) {
        if (flags & O_CREATE) {
            return -1;
        }
        file->file = ATTR_DIRECTORY;
        // k_memcpy(file->name,cur_dir.name,k_strlen(cur_dir.name)+1);
        file->dev = 0; // TODO
        file->flags = flags & ~(O_DIRECTORY | O_CREATE);
        file->mode = mode;
        file->first_cluster = new.first_cluster;
        file->inode = (ptr_t) new.node;
        file->pos = 0;
        file->size = fat32_fcluster2size(cur_dir.size);
        return ret;
    }
    int offset = 0;
    dentry_t *dtable = NULL;
    dtable = (dentry_t *)new.node->i_mapping;
    offset = fat32_name2offset(name, dtable);
    // k_print("[debug] hit dir inode %d, offset %d, cond %d\n",new.node->i_ino,offset,((dtable[offset].sn.attr & ATTR_DIRECTORY) == 0) == ((flags & O_DIRECTORY) == 0));
    if (offset >= 0) {
        if (((dtable[offset].sn.attr & ATTR_DIRECTORY) == 0) == ((flags & O_DIRECTORY) == 0)) {
            // k_print("[debug] openat file type right!\n");
            // TODO if inode has create
            fat32_dentry2fd(&dtable[offset], file, flags & ~O_CREATE, mode, name);
            uint16_t new_inode = 0;
            if (dtable[offset].sn.nt_res == 0) {
                new_inode = alloc_inode(&dtable[offset], name, new.node->i_ino, offset);
            } else {
                new_inode = dtable[offset].sn.fst_clus_lo;
            }
            file->inode = (ptr_t)&inode_table[new_inode];
            if (use_fd) {
                fd_t *file = get_fd(dirfd);
                file->size = fat32_fcluster2size(file->first_cluster);
            }
            return ret;
        }
    } else if (offset < 0 && (flags & O_CREATE)) {
        // k_print("[debug] openat create file with name %s!\n",name);
        offset = alloc_dentry(&dtable, name, flags & ~O_CREATE, mode, &new);
        fat32_dentry2fd(&dtable[offset], file, flags & ~O_CREATE, mode, name);
        uint16_t new_inode = alloc_inode(&dtable[offset], name, new.node->i_ino, offset);
        file->inode = (ptr_t)&inode_table[new_inode];
        if (use_fd) {
            fd_t *file = get_fd(dirfd);
            file->size = fat32_fcluster2size(file->first_cluster);
        }
        return ret;
    }

    fd_free(ret);
    return -1;
}

long sys_close(int fd) {
    // if(fd>=STDMAX && fd < MAX_FD)
    //     return fd_free(fd);
    fd_free(fd);
    return 0;
}

/**
 * @brief  allocate pipe to fd[2].
 * @param  fd: target fd array.
 * @retval  -1: failed
 *          0: succeed
 */
long sys_pipe2(int *fd, mode_t flags) {
    // 1. alloc one pipe? what's pipe?
    // 2. alloc 2 fd for pipe
    if (pipe_alloc(fd) == -1) {
        return -1;
    }
    int mailbox = k_mbox_open(fd[0], fd[1]);
    // init
    fd_t *file = get_fd(fd[0]);
    // k_print("[pipe] get pipe read id %u, fd %d\n", file->pip_num, fd[0]);
    file->file = ATTR_DIRECTORY >> 1;
    file->pos = 0;
    file->flags = O_RDONLY;
    file->mailbox = mailbox;

    file = get_fd(fd[1]);
    // k_print("[pipe] get pipe write id %u, fd %d\n", file->pip_num, fd[1]);
    file->file = ATTR_DIRECTORY >> 1;
    file->pos = 0;
    file->flags = O_WRONLY;
    file->mailbox = mailbox;

    pipe_t *p = &pipe_table[file->pip_num];
    p->r_valid = 1;
    p->w_valid = 1;
    p->mailbox = mailbox;

    return 0;
}

long sys_getdents64(int fd, dirent64_t *dirent, size_t len) {
    fd_t *file = get_fd(fd);
    if (file->file != ATTR_DIRECTORY) {
        return -1;
    }
    // TODO change file descriptor
    //  dentry_t *dtable = read_whole_dir(file->first_cluster, 0);
    dentry_t *dtable = (dentry_t *)((inode_t *)file->inode)->i_mapping;
    char name[MAX_NAME_LEN];
    int offset = 0;
repeat:
    offset = dentry2name(&dtable[file->pos], name);
    if (!offset) {
        return 0;
    }
    if ((offset == 1) && (name[0] == '\0')) {
        goto repeat;
    }
    file->pos += offset;
    dirent->d_ino = 0;
    dirent->d_off = (long)offset;
    dirent->d_reclen = sizeof(dirent64_t) + k_strlen(name) + 1;
    dirent->d_type = dtable[file->pos - 1].sn.attr;
    k_memcpy(dirent->d_name, name, k_strlen(name) + 1);
    return sizeof(dirent64_t) + k_strlen(name) + 1;
}

#define SEEK_SET 0 /* seek relative to beginning of file */
#define SEEK_CUR 1 /* seek relative to current file position */
#define SEEK_END 2 /* seek relative to end of file */
long sys_lseek(unsigned int fd, off_t offset, unsigned int whence) {
    fd_t *file = get_fd(fd);
    if (!file) {
        return 0;
    }
    if (whence == SEEK_SET) {
        if (file->size > offset) {
            file->pos = offset;
            return file->pos;
        }
        file->pos = file->size;
    } else if (whence == SEEK_CUR) {
        if (file->size > offset + file->pos) {
            file->pos += offset;
            return file->pos;
        }
        file->pos = file->size;
    } else if (whence == SEEK_END) {
        if (offset < file->size) {
            file->pos = file->size - offset;
            return file->pos;
        }
        file->pos = 0;
    }
    return file->pos;
}

/**
 * @brief  read count bytes to buf of fd file
 * @param  fd     file descripter
 * @param  buf    bytes buf
 * @param  count  read len
 */
ssize_t sys_read(int fd, char *buf, size_t count) {
    // if (fd == stdin) {
    //     *buf = k_port_read();
    //     return 1;
    // }

    fd_t *file = get_fd(fd);
    if (!file) {
        return -1;
    }

    if (file->file == stdin) {
        *buf = k_port_read();
        return 1;
    }

    if (file->flags << 31 != 0) {
        return -1;
    }
    if (file->flags & O_DIRECTORY) {
        return -1;
    }

    // for pipe
    if (file->is_pipe_read) {
        pipe_t *p = &pipe_table[file->pip_num];
        int ret = 0;
        mbox_arg_t arg = {.msg = (void *)buf, .msg_length = count, .sleep_operation = 0};
        ret = k_mbox_recv(p->mailbox, NULL, &arg);
        return ret;
    }
    // TODO change file descriptor
    //  uint8_t *data = read_whole_dir(file->first_cluster, file->size);
    //  k_print("[debug] read print file %s\n",(char *)((inode_t *)file->inode)->i_mapping);
    uint8_t *data = (uint8_t *)((inode_t *)file->inode)->i_mapping;
    if (file->size - file->pos >= count) {
        k_memcpy((uint8_t *)buf, &data[file->pos], count);
        file->pos += count;
        // free data TODO
        return count;
    } else {
        int len = file->size - file->pos;
        k_memcpy((uint8_t *)buf, &data[file->pos], len);
        file->pos = file->size;
        // free data TODO
        return len;
    }
}

ssize_t sys_write(int fd, const char *buf, size_t count) {
    // k_print("[debug] sys_write fd %d\n",fd);
    // if (fd == STDOUT) {
    //     sys_screen_write_len((char *)buf, count);
    //     return count;
    // }
    fd_t *file = get_fd(fd);
    if (!file) {
        return -1;
    }
    // k_print("[debug] sys_write with fd %d\n",fd);
    if (file->file == stdout) {
        sys_screen_write_len((char *)buf, count);
        return count;
    } else if (file->file == stderr) {
        sys_screen_write("[ERROR] ");
        sys_screen_write_len((char *)buf, count);
        return count;
    }

    if ((file->flags << 30) == 0) {
        // k_print("[debug] sys_write flags error with fd %d\n",fd);
        return -1;
    }
    if (file->file & ATTR_DIRECTORY) {
        // k_print("[debug] sys_write flags DIR with fd %d\n",fd);
        return -1;
    }
    // for pipe
    if (file->is_pipe_write) {
        pipe_t *p = &pipe_table[file->pip_num];
        mbox_arg_t arg = {.msg = (void *)buf, .msg_length = count, .sleep_operation = 0};
        // k_print("[debug] pipe %d mailbox %d write %s!\n",file->pip_num,file->mailbox,buf);
        int ret = k_mbox_send(p->mailbox, NULL, &arg);
        p->w_valid = 0;
        return ret;
    }

    // k_print("[debug] sys_write not pipe write with fd %d\n",fd);
    int new = (file->pos + count + fat.bytes_per_cluster - 1) / fat.bytes_per_cluster - (file->size + fat.bytes_per_cluster - 1) / fat.bytes_per_cluster; // need more cluster?
    int reload = 0;
    if (new) {
        reload = 1;
    }
    // k_print("[debug] alloc %d clusters!\n",new);
    while (new > 0) {
        file->first_cluster = alloc_cluster(file->first_cluster);
        if (!((inode_t *)file->inode)->i_fclus) {
            ((inode_t *)file->inode)->i_fclus = file->first_cluster;
        }
        new --;
    }
    if (reload) {
        ((inode_t *)file->inode)->i_mapping = (ptr_t)read_whole_dir(file->first_cluster, 0);
    }
    // if out of size
    if (file->pos + count > file->size) {
        file->size = file->pos + count;
        // k_print("[debug] file size %d\n",file->size);
    }
    update_dentry_size(((inode_t *)file->inode)->i_upper, ((inode_t *)file->inode)->i_offset, file->size);
    uint8_t *data = (uint8_t *)((inode_t *)file->inode)->i_mapping;
    if (!data) {
        return -1;
    }
    k_memcpy(&data[file->pos], (uint8_t *)buf, count); // has enough room
    // k_print("[debug] write %s\n",buf);
    // write_whole_dir(file->first_cluster, data, 1);
    file->pos += count;
    return count;
}

long sys_readv(unsigned long fd, const iovec_t *vec, unsigned long vlen) {
    int retval = 0;
    for (int i = 0; i < vlen; i++) {
        retval += sys_read(fd, (char *)vec[i].iov_base, vec[i].iov_len);
    }
    return retval;
}

long sys_writev(unsigned long fd, const iovec_t *vec, unsigned long vlen) {
    int retval = 0;
    for (int i = 0; i < vlen; i++) {
        retval += sys_write(fd, (const char *)vec[i].iov_base, vec[i].iov_len);
    }
    return retval;
}

long sys_sendfile64(int out_fd, int in_fd, loff_t *offset, size_t count) {
    char *buf = k_mm_malloc(count + 1);
    fd_t *file = get_fd(in_fd);
    if (offset) {
        file->pos = *offset;
    }
    ssize_t read_len = sys_read(in_fd, buf, count);
    if (offset) {
        *offset = *offset + (loff_t)read_len;
    }
    file = get_fd(out_fd);
    sys_write(out_fd, buf, read_len);
    return read_len;
}

long sys_readlinkat(int dfd, const char *path, char *buf, int bufsiz) {
    return 0;
}
// TODO
long sys_newfstatat(int dfd, const char *filename, stat_t *statbuf, int flag) {
    fd_t *file = get_fd(dfd);
    if (!file && dfd != AT_FDCWD) {
        return -1;
    }
    // get dir file belonged to
    dir_info_t new;
    if (!file) {
        new.first_cluster = cur_dir.first_cluster;
        new.node = cur_dir.node;
        new.size = cur_dir.size;
    } else {
        new.first_cluster = file->first_cluster;
        new.node = (inode_t *)file->inode;
        new.size = file->size;
    }
    char path[MAX_PATH_LEN], name[MAX_NAME_LEN];
    int ret = 0;
    filename2path(path, name, filename);
    ret = fat32_path2dir(path, &new, new);
    // not found by path
    if (!ret) {
        // k_memset(statbuf,0,sizeof(statbuf));
        return -2;
    }
    if (!k_strcmp(".", name)) {
        statbuf->st_dev = file->dev;
        statbuf->st_ino = ((inode_t *)file->inode)->i_ino;
        statbuf->st_mode = file->mode;
        statbuf->st_nlink = 1;
        statbuf->st_uid = file->uid;
        statbuf->st_gid = file->gid;
        statbuf->st_rdev = file->rdev;
        // statbuf->__pad = 0;
        statbuf->st_size = file->size;
        statbuf->st_blksize = fat.bpb.bytes_per_sec;
        statbuf->__pad2 = 0;
        statbuf->st_blocks = (uint64_t)(fat32_fcluster2size(file->first_cluster) / fat.bpb.bytes_per_sec);
        nanotime_val_t time;
        k_time_get_nanotime(&time);
        // statbuf->st_atime_sec = time.sec;
        // statbuf->st_atime_nsec = time.nsec;
        // statbuf->st_mtime_sec = time.sec;
        // statbuf->st_mtime_nsec = time.nsec;
        // statbuf->st_ctime_sec = time.sec;
        // statbuf->st_ctime_nsec = time.nsec;
        // statbuf->__unused[0] = 0;
        // statbuf->__unused[1] = 0;
    }
    int offset = 0;
    dentry_t *dtable = NULL;
    dtable = (dentry_t *)new.node->i_mapping;
    offset = fat32_name2offset(name, dtable);
    if (offset < 0) {
        // k_memset(statbuf,0,sizeof(statbuf));
        return -2;
    }
    // TODO correct right information
    uint16_t new_inode;
    if (dtable[offset].sn.nt_res == 0) {
        new_inode = alloc_inode(&dtable[offset], name, new.node->i_ino, offset);
    } else {
        new_inode = dtable[offset].sn.fst_clus_lo;
    }
    // statbuf->st_dev = file->dev;
    statbuf->st_ino = new_inode;
    statbuf->st_mode = dtable[offset].sn.attr;
    statbuf->st_nlink = 1;
    // statbuf->st_uid = file->uid;
    // statbuf->st_gid = file->gid;
    // statbuf->st_rdev = file->rdev;
    // statbuf->__pad = 0;
    statbuf->st_size = dtable[offset].sn.file_size;
    statbuf->st_blksize = fat.bpb.bytes_per_sec;
    statbuf->__pad2 = 0;
    statbuf->st_blocks = (uint64_t)(fat32_fcluster2size(fat32_dentry2fcluster(&dtable[offset])) / fat.bpb.bytes_per_sec);
    nanotime_val_t time;
    k_time_get_nanotime(&time);
    // statbuf->st_atime_sec = time.sec;
    // statbuf->st_atime_nsec = time.nsec;
    // statbuf->st_mtime_sec = time.sec;
    // statbuf->st_mtime_nsec = time.nsec;
    // statbuf->st_ctime_sec = time.sec;
    // statbuf->st_ctime_nsec = time.nsec;
    // statbuf->__unused[0] = 0;
    // statbuf->__unused[1] = 0;
    return 0;
}

// TODO
long sys_newfstat(unsigned int fd, stat_t *statbuf) {
    fd_t *file = get_fd(fd);
    // statbuf->st_dev = file->dev;
    // statbuf->st_ino = fd - 3;
    // statbuf->st_mode = file->mode;
    // statbuf->st_nlink = file->nlink;
    // statbuf->st_uid = file->uid;
    // statbuf->st_gid = file->gid;
    // statbuf->st_rdev = file->rdev;
    // statbuf->__pad = 0;
    // statbuf->st_size = file->size;
    // statbuf->st_blksize = fat.bpb.bytes_per_sec;
    // statbuf->__pad2 = 0;
    // statbuf->st_blocks = (uint64_t)(fat32_fcluster2size(file->first_cluster) / fat.bpb.bytes_per_sec);
    // statbuf->st_atime_sec = file->atime_sec;
    // statbuf->st_atime_nsec = file->atime_nsec;
    // statbuf->st_mtime_sec = file->mtime_sec;
    // statbuf->st_mtime_nsec = file->mtime_nsec;
    // statbuf->st_ctime_sec = file->ctime_sec;
    // statbuf->st_ctime_nsec = file->ctime_nsec;
    if (!file) {
        return -1;
    }
    statbuf->st_dev = file->dev;
    if (file->file > STDMAX) {
        statbuf->st_ino = ((inode_t *)file->inode)->i_ino;
    }
    statbuf->st_mode = file->mode;
    statbuf->st_nlink = 1;
    statbuf->st_uid = file->uid;
    statbuf->st_gid = file->gid;
    statbuf->st_rdev = file->rdev;
    // statbuf->__pad = 0;
    statbuf->st_size = file->size;
    statbuf->st_blksize = fat.bpb.bytes_per_sec;
    statbuf->__pad2 = 0;
    statbuf->st_blocks = (uint64_t)(fat32_fcluster2size(file->first_cluster) / fat.bpb.bytes_per_sec);
    nanotime_val_t time;
    k_time_get_nanotime(&time);
    // statbuf->st_atime_sec = time.sec;
    // statbuf->st_atime_nsec = time.nsec;
    // statbuf->st_mtime_sec = time.sec;
    // statbuf->st_mtime_nsec = time.nsec;
    // statbuf->st_ctime_sec = time.sec;
    // statbuf->st_ctime_nsec = time.nsec;
    // statbuf->__unused[0] = 0;
    // statbuf->__unused[1] = 0;
    // k_print("[debug] fd size %d, file ino %d\n",file->size,statbuf->st_ino);
    return 0;
}

long sys_fstat64(int fd, kstat64_t *statbuf) {
    fd_t *file = get_fd(fd);
    // statbuf->st_dev = file->dev;
    // statbuf->st_ino = fd - 3;
    // statbuf->st_mode = file->mode;
    // statbuf->st_nlink = file->nlink;
    // statbuf->st_uid = file->uid;
    // statbuf->st_gid = file->gid;
    // statbuf->st_rdev = file->rdev;
    // statbuf->__pad = 0;
    // statbuf->st_size = file->size;
    // statbuf->st_blksize = fat.bpb.bytes_per_sec;
    // statbuf->__pad2 = 0;
    // statbuf->st_blocks = (uint64_t)(fat32_fcluster2size(file->first_cluster) / fat.bpb.bytes_per_sec);
    // statbuf->st_atime_sec = file->atime_sec;
    // statbuf->st_atime_nsec = file->atime_nsec;
    // statbuf->st_mtime_sec = file->mtime_sec;
    // statbuf->st_mtime_nsec = file->mtime_nsec;
    // statbuf->st_ctime_sec = file->ctime_sec;
    // statbuf->st_ctime_nsec = file->ctime_nsec;
    statbuf->st_dev = file->dev;
    statbuf->st_ino = ((inode_t *)file->inode)->i_ino;
    statbuf->st_mode = file->mode;
    statbuf->st_nlink = 1;
    statbuf->st_uid = file->uid;
    statbuf->st_gid = file->gid;
    statbuf->st_rdev = file->rdev;
    statbuf->__pad = 0;
    statbuf->st_size = file->size;
    statbuf->st_blksize = fat.bpb.bytes_per_sec;
    statbuf->__pad2 = 0;
    statbuf->st_blocks = (uint64_t)(fat32_fcluster2size(file->first_cluster) / fat.bpb.bytes_per_sec);
    nanotime_val_t time;
    k_time_get_nanotime(&time);
    statbuf->st_atime_sec = time.sec;
    statbuf->st_atime_nsec = time.nsec;
    statbuf->st_mtime_sec = time.sec;
    statbuf->st_mtime_nsec = time.nsec;
    statbuf->st_ctime_sec = time.sec;
    statbuf->st_ctime_nsec = time.nsec;
    statbuf->__unused[0] = 0;
    statbuf->__unused[1] = 0;
    return 0;
}

long sys_fstatat64(int fd, const char *filename, kstat64_t *statbuf, int flag) {
    return 0;
}

long sys_sync(void) {
    return 0;
}

long sys_fsync(unsigned int fd) {
    return 0;
}

long sys_umask(int mask) {
    return 0;
}

long sys_syncfs(int fd) {
    return 0;
}

long sys_renameat2(int olddfd, const char *oldname, int newdfd, const char *newname, unsigned int flags) {
    return 0;
}

long sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    if (length <= 0 || (uintptr_t)addr % NORMAL_PAGE_SIZE != 0) {
        return -EINVAL;
    }
    if (offset % NORMAL_PAGE_SIZE != 0) {
        offset = offset - offset % NORMAL_PAGE_SIZE + NORMAL_PAGE_SIZE;
    }
    if ((uint64_t)addr % NORMAL_PAGE_SIZE != 0) {
        addr = addr - (uint64_t)addr % NORMAL_PAGE_SIZE + NORMAL_PAGE_SIZE;
    }
    // uintptr_t ret = (uintptr_t)addr;
    if (!addr) {
        // ret = sys_brk(0);
        // ret = sys_brk(K_ROUND(ret, NORMAL_PAGE_SIZE));
        // sys_brk(ret + K_ROUND(length, NORMAL_PAGE_SIZE));
        addr = (void *)k_mm_alloc_newva((length / NORMAL_PAGE_SIZE) + 1);
    }
    uintptr_t ret = (uintptr_t)addr;
    uintptr_t tmp = (uintptr_t)addr;
    uintptr_t start_kva = 0;
    bool first = true;
    while (length) {
        if (first) {
            start_kva = k_mm_alloc_page_helper(tmp, (pa2kva((*current_running)->pgdir << 12)));
            first = false;
        } else {
            k_mm_alloc_page_helper(tmp, (pa2kva((*current_running)->pgdir << 12)));
        }
        length -= NORMAL_PAGE_SIZE;
        tmp += NORMAL_PAGE_SIZE;
    }
    if (flags & MAP_ANONYMOUS) {
        if (fd != -1) {
            return -EBADF;
        }
    } else {
        fd_t *file = get_fd(fd);
        uint8_t *data = (uint8_t *)((inode_t *)file->inode)->i_mapping;
        // TODO change file descriptor
        // uint8_t *data = read_whole_dir(file->first_cluster, file->size);
        // k_print("[debug] mmap %s\n",&data[offset]);
        k_memcpy((uint8_t *)start_kva, &data[offset], length);
    }
    return (long)ret;
}

long sys_munmap(unsigned long addr, size_t len) {
    // TODO clear page table!
    return 0;
}

long sys_mremap(unsigned long addr, unsigned long old_len, unsigned long new_len, unsigned long flags, unsigned long new_addr) {
    return 0;
}

long sys_utimensat(int dfd, const char *filename, kernel_timespec_t *utimes, int flags) {
    return 0;
}

long sys_symlink(int dfd, const char *filename) {
    if (fs_check_file_existence(filename)) {
        return -1;
    }
    int fd = sys_openat(AT_FDCWD, filename, O_CREATE, 0);
    fd_free(fd);
    return 0;
}
// int fs_start_sec = 114514;
// int magic_number = 0x114514;
// int sb_sec_offset = 0;
// int sb2_sec_offset = 1024 * 1024 + 1;

// uint64_t iab_map_addr_offset = 0;
// uint64_t sec_map_addr_offset = 512;
// uint64_t iab_map_addr_size = 257 * 512;

// uint64_t inode_addr_offset = 258 * 512;
// uint64_t dir_addr_offset = 770 * 512;
// uint64_t dir2_addr_offset = 778 * 512;
// uint64_t dir3_addr_offset = 786 * 512;
// uint64_t dir4_addr_offset = 794 * 512;
// uint64_t data_addr_offset = 802 * 512;
// uint64_t empty_block = FS_KERNEL_ADDR - 0x2000;

// inode_t nowinode;
// int nowinodeid;

// fentry_t fd[20];
// int nowfid = 0;
// int freefid[20];
// int freenum = 0;