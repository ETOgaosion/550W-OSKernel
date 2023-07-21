#include <asm/pgtable.h>
#include <asm/sbi.h>
// #include <fs/fs.h>
#include <arch/riscv/include/asm/io.h>
#include <drivers/screen/screen.h>
#include <drivers/virtio/virtio.h>
#include <fs/fat32.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/mm.h>
#include <os/pcb.h>
#include <os/sync.h>
#include <os/time.h>
#include <user/user_programs.h>

// fat32 infomation
fat32_t fat;
dir_info_t root_dir = {.name = "/\0"}, cur_dir = {.name = "/\0"};

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

// TODO : func of free buffers by &data, use write_whole_dir
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
        k_print("[ERROR] read empty file!\n");
        return NULL;
    }
    uint32_t ftable[fat.bpb.bytes_per_sec / sizeof(uint32_t)];
    if (size == 0) {
        size = fat32_fcluster2size(first);
    }
    // k_print("size = %d, malloc %d sec bytes\n", size, (size / fat.bpb.bytes_per_sec + 1) * fat.bpb.bytes_per_sec);
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
                d_sd_write((char *)ftable2, &base, 1);
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
        d_sd_write((char *)ftable1, &cur_fat_sec, 1);
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
    k_memcpy((uint8_t *)path, (uint8_t *)filename, tag);
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
                    name[long_entry_num * LONG_NAME_LEN + count] = '0';
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

    dtable = (dentry_t *)read_whole_dir(now->first_cluster, 0);

    offset = fat32_name2offset(name, dtable);

    if (offset < 0 || dtable[offset].sn.attr != ATTR_DIRECTORY) {
        // free dtable, nodir with same name
        return 0;
    }

    now->first_cluster = fat32_dentry2fcluster(&dtable[offset]);
    now->size = dtable[offset].sn.file_size;
    // get path after return

    // TODO:free dtable
    return 1;
}

/**
 * @brief  set new dentry
 * @param  name  file name
 * @param  flags file flags
 * @param  len length of LFN
 *
 */
int fat32_new_dentry(dentry_t *entry, uint32_t flags, int len, char *name) {
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
    while (len > 0) {
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
        entry[i++].ln.order = (uint8_t)len;
        len--;
    }
    entry->ln.order |= LAST_LONG_ENTRY;

    if (!(flags & O_DIRECTORY)) {
        return 0;
    }

    // TODO : set ./.. in dir entry, not finish
    dentry_t *dtable = read_whole_dir(fat32_dentry2fcluster(&entry[len_0]), 0);
    k_bzero(dtable, fat.bytes_per_cluster);
    // dtable[0].sn.name1[0] = '.';
    // dtable[0].sn.name1[1] = ' ';
    // dtable[0].sn.name2[0] = ' ';
    // dtable[0].sn.attr = ATTR_DIRECTORY;

    // dtable[1].sn.name1[0] = '.';
    // dtable[1].sn.name1[1] = '.';
    // dtable[1].sn.name1[2] = ' ';
    // dtable[1].sn.name2[0] = ' ';
    // dtable[1].sn.attr = ATTR_DIRECTORY;

    return write_whole_dir(fat32_dentry2fcluster(&entry[len_0]), dtable, 1);
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

            fat32_new_dentry(&table[i - len], flags, len, name);
            break;
        } else {
            cnt = 0;
        }
        if (cnt > len) {
            fat32_new_dentry(&table[i - len], flags, len, name);
            break;
        }
        i++;
    }
    if (i >= size) {
        // k_print("[debug] openat alloc new cluster!\n");
        alloc_cluster(now->first_cluster);
        now->size += fat.bytes_per_cluster;
        // reload dir table and go to the head of this function?
        *dtable = read_whole_dir(now->first_cluster, 0);
        // free outside
        table = *dtable;
        // add dentry
        fat32_new_dentry(&table[size], flags, len, name);
        i = len + size;
    }
    write_whole_dir(now->first_cluster, *dtable, 1);
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
        // generate name later as we can't get path from root or cur
        // k_memcpy(new->name,path_0,kk_strlen(path)+1);
        return 1;
    }
}

int fat32_dentry2fd(dentry_t *entry, fd_t *file, mode_t flags, mode_t mode, char *name) {
    if (entry->sn.attr & ATTR_DIRECTORY) {
        file->file = ATTR_DIRECTORY;
    } else {
        file->file = ATTR_DIRECTORY >> 1;
    }
    k_memcpy((uint8_t *)file->name, (uint8_t *)name, k_strlen(name) + 1);
    file->dev = 0; // TODO
    file->flags = flags & ~(O_DIRECTORY);
    file->mode = mode;
    file->first_cluster = fat32_dentry2fcluster(entry);
    file->cur_clus_num = file->first_cluster;
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
    d_sd_write((char *)boot, &boot_sec, 1);
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

    // set root dir and cwd
    root_dir.first_cluster = fat.bpb.root_cluster;
    root_dir.size = fat32_fcluster2size(root_dir.first_cluster);
    root_dir.name[0] = '/';
    root_dir.name[1] = '\0';
    k_print("root dir begin at cluster %d, size = %d\n", root_dir.first_cluster);
    cur_dir.first_cluster = root_dir.first_cluster;
    cur_dir.size = root_dir.size;
    cur_dir.name[0] = '/';
    cur_dir.name[1] = '\0';
    // kfree(boot);

    dentry_t *dtable = read_whole_dir(root_dir.first_cluster, 0);
    char name[MAX_NAME_LEN];
    int offset = 0;
    offset += dentry2name(&dtable[offset], name);

    offset = dentry2name(&dtable[offset], name);

    // //[read] ok
    // int read_fd = sys_openat(AT_FDCWD,"./text.txt",O_RDONLY,0);
    // fd_t *file_txt = get_fd(read_fd);
    // if(file_txt)
    //     k_print("[test] 1. read openat success! name %s\n",file_txt->name);
    // int read_len = sys_read(read_fd,name,256);
    // // k_print("[test] 2. read %d bytes : %s\n",read_len,name);
    // sys_write(STDOUT,name,read_len);
    // fd_free(read_fd);

    // //[chdir] ok(cwd is path or name?)
    // sys_mkdirat(AT_FDCWD,"test_chdir", 0);
    // sys_chdir("test_chdir");
    // sys_getcwd(name,MAX_NAME_LEN);
    // k_print("[test] getcwd %s",name);

    // // [dup] ok
    // int dup_fd = sys_dup(STDOUT);
    // k_print("[test] dup fd %d\n",dup_fd);

    // // [dup] ok
    // sys_dup3(STDOUT,100,0);
    // const char *str = "  from fd 100\n";
    // sys_write(100, str, k_strlen(str));

    // // [fstats] ok
    // long sys_fd = sys_openat(AT_FDCWD,"./text.txt",O_RDONLY,0);
    // kstat_t kst;
    // sys_fstat(sys_fd, &kst);
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
int fs_load_file(const char *name, uint8_t **bin, int *len) {
    dentry_t *dtable = NULL;
    // TODO: read in dir data
    dtable = read_whole_dir(root_dir.first_cluster, 0);
    int offset = fat32_name2offset((char *)name, dtable);
    if (offset < 0) {
        // k_print("[debug] can't load, no such file!\n");
        return -1;
    }
    uint32_t first = fat32_dentry2fcluster(&dtable[offset]);
    // free buffer of dtable
    *len = dtable[offset].sn.file_size;
    if (*len == 0) {
        // k_print("[debug] can't load, empty file!\n");
        return -1;
    }
    *bin = (uint8_t *)read_whole_dir(first, 0);
    return 0;
}

// [FUNCTION REQUIREMENTS]
bool fs_check_file_existence(const char *name) {
    return true;
}

// [TODO]
long sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg) {
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

long sys_lsetxattr(const char *path, const char *name, const void *value, size_t, int flags) {
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
    k_memcpy((uint8_t *)buf, (uint8_t *)cur_dir.name, min(k_strlen(cur_dir.name) + 1, size));
    return (long)buf;
}

/**
 * @brief  copy a fd and return the new one
 * @param  old old fd
 *
 */
long sys_dup(int old) {
    fd_t *file1 = get_fd(old);
    if (!file1) {
        return -1;
    }
    int new = fd_alloc();
    if (new < 0) {
        return -1;
    }
    fd_t *file2 = get_fd(new);
    if (!file2) {
        return -1;
    }
    k_memcpy((uint8_t *)file2, (uint8_t *)file1, sizeof(fd_t));
    return new; // fail return -1
}

/**
 * @brief  copy a fd and return the new one
 * @param  old old fd
 * @param  new new fd
 */
long sys_dup3(int old, int new, mode_t flags) {
    fd_t *file1 = get_fd(old);
    if (!file1) {
        return -1;
    }
    fd_t *file2 = fd_alloc_spec(new);
    if (!file2) {
        return -1;
    }
    k_memcpy((uint8_t *)file2, (uint8_t *)file1, sizeof(fd_t));
    return new; // fail return -1
}

long sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg) {
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
        new.first_cluster = file->first_cluster;
        new.size = fat32_fcluster2size(new.first_cluster);
        // no use of name
    }

    // not found by path
    if (!ret) {
        return -1;
    }

    int offset = 0;
    dentry_t *dtable = NULL;

    dtable = read_whole_dir(new.first_cluster, 0);

    offset = fat32_name2offset(name, dtable);

    if (offset >= 0) {
        return -1;
    }

    // k_print("[fat32] openat create file with name %s!\n",name);
    offset = alloc_dentry(&dtable, name, O_DIRECTORY | O_RDWR, mode, &new);
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
        new.size = fat32_fcluster2size(new.first_cluster);
        // no use of name
    }

    // not found by path
    if (!ret) {
        return -1;
    }

    // try open file
    int offset = 0;
    dentry_t *dtable = NULL;

    dtable = read_whole_dir(new.first_cluster, 0);

    offset = fat32_name2offset(name, dtable);
    if (offset < 0) {
        // k_print("[debug] : testopen not found!\n");
        // free dtable
        return -1;
    }
    if (!(((dtable[offset].sn.attr & ATTR_DIRECTORY) == 0) ^ ((flags & AT_REMOVEDIR) == 0))) {
        // k_print("[debug] destroy %s at dir %s, offset %d\n",name,path,offset);
        // TODO: set empty entry
        destroy_dentry(dtable, offset);
        write_whole_dir(new.first_cluster, dtable, 1);
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
    return 0;
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
        k_memcpy((uint8_t *)cur_dir.name, (uint8_t *)path, k_strlen(path) + 1);
    } else {
        // TODO : fix '/' error
        k_memcpy((uint8_t *)(&cur_dir.name[k_strlen(cur_dir.name)]), (uint8_t *)path, k_strlen(path) + 1);
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
    // get dir file belonged to
    dir_info_t new;
    char path[MAX_PATH_LEN], name[MAX_NAME_LEN];
    int ret = 0;
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
        new.first_cluster = file->first_cluster;
        new.size = fat32_fcluster2size(file->first_cluster);
        ret = 1;
        // no use of name
    }
    // not found by path
    if (!ret) {
        return -1;
    }

    // try open file
    ret = fd_alloc();
    fd_t *file = get_fd(ret);
    // k_print("[debug] name = %s\n",name);
    if (!k_strcmp(".", name)) {
        file->file = ATTR_DIRECTORY;
        // k_memcpy((uint8_t *)file->name,(uint8_t *)cur_dir.name,k_strlen(cur_dir.name)+1);
        file->dev = 0; // TODO
        file->flags = flags & ~(O_DIRECTORY | O_CREATE);
        file->mode = mode;
        file->first_cluster = new.first_cluster;
        file->cur_clus_num = new.first_cluster;
        file->pos = 0;
        file->size = fat32_fcluster2size(cur_dir.size);
        return ret;
    }
    int offset = 0;
    dentry_t *dtable = NULL;
    dtable = read_whole_dir(new.first_cluster, 0);

    offset = fat32_name2offset(name, dtable);

    if (offset >= 0) {
        if (!(((dtable[offset].sn.attr & ATTR_DIRECTORY) == 0) ^ ((flags & O_DIRECTORY) == 0))) {
            // k_print("[debug] openat file type right!\n");
            fat32_dentry2fd(&dtable[offset], file, flags & ~O_CREATE, mode, name);
            return ret;
        }
    } else if (offset < 0 && (flags & O_CREATE)) {
        // k_print("[debug] openat create file with name %s!\n",name);
        offset = alloc_dentry(&dtable, name, flags & ~O_CREATE, mode, &new);
        fat32_dentry2fd(&dtable[offset], file, flags & ~O_CREATE, mode, name);
        return ret;
    }

    fd_free(ret);
    return -1;
}

long sys_close(int fd) {
    return fd_free(fd);
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
    dentry_t *dtable = read_whole_dir(file->first_cluster, 0);
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
    k_memcpy((uint8_t *)dirent->d_name, (uint8_t *)name, k_strlen(name) + 1);
    return sizeof(dirent64_t) + k_strlen(name) + 1;
}

long sys_lseek(unsigned int fd, off_t offset, unsigned int whence) {
    return 0;
}

/**
 * @brief  read count bytes to buf of fd file
 * @param  fd     file descripter
 * @param  buf    bytes buf
 * @param  count  read len
 */
ssize_t sys_read(int fd, char *buf, size_t count) {
    fd_t *file = get_fd(fd);
    if (!file) {
        return -1;
    }
    if (file->file == stdin) {
        *buf = k_port_read();
        return 1;
    }

    if (!(file->flags == O_RDONLY) && !(file->flags == O_RDWR)) {
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

    uint8_t *data = read_whole_dir(file->first_cluster, file->size);
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
    fd_t *file = get_fd(fd);
    if (!file) {
        return -1;
    }
    if (file->file == stdout) {
        // TODO : create stdin/out/err
        sys_screen_write_len((char *)buf, count);
        return count;
    }

    if (!(file->flags == O_WRONLY) && !(file->flags == O_RDWR)) {
        return -1;
    }
    if (file->file == ATTR_DIRECTORY) {
        return -1;
    }
    // for pipe
    if (file->is_pipe_write) {
        pipe_t *p = &pipe_table[file->pip_num];
        mbox_arg_t arg = {.msg = (void *)buf, .msg_length = count, .sleep_operation = 0};
        int ret = k_mbox_send(p->mailbox, NULL, &arg);
        p->w_valid = 0;
        return ret;
    }

    int new = (file->pos + count + fat.bytes_per_cluster - 1) / fat.bytes_per_cluster - (file->size + fat.bytes_per_cluster - 1) / fat.bytes_per_cluster; // need more cluster?
    while (new > 0) {
        file->first_cluster = alloc_cluster(file->first_cluster);
        new --;
    }
    uint8_t *data = read_whole_dir(file->first_cluster, 0); // enough room
    if (!data) {
        return -1;
    }
    if (file->pos + count > file->size) {
        file->size = file->pos + count;
    }
    k_memcpy(&data[file->pos], (uint8_t *)buf, count); // has enough room
    write_whole_dir(file->first_cluster, data, 1);
    file->pos += count;
    return count;
}

long sys_readv(unsigned long fd, const iovec_t *vec, unsigned long vlen) {
    return 0;
}

long sys_writev(unsigned long fd, const iovec_t *vec, unsigned long vlen) {
    return 0;
}

long sys_sendfile64(int out_fd, int in_fd, loff_t *offset, size_t count) {
    return 0;
}

long sys_readlinkat(int dfd, const char *path, char *buf, int bufsiz) {
    return 0;
}

long sys_newfstatat(int dfd, const char *filename, stat_t *statbuf, int flag) {
    return 0;
}

long sys_newfstat(unsigned int fd, stat_t *statbuf) {
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
    statbuf->st_ino = fd - 3;
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

// [NOTE]: first argument file_t struct defined in file.h
long sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    if (addr != NULL) { // NOT support spec addr
        return (long)addr;
    }
    fd_t *file = get_fd(fd);
    if (!file) {
        return (long)-1;
    }
    uintptr_t ret = k_mm_alloc_newva();
    uintptr_t kva = k_mm_alloc_page_helper(ret, (pa2kva((*current_running)->pgdir)));

    uint8_t *data = read_whole_dir(file->first_cluster, file->size);
    k_memcpy((uint8_t *)kva, &data[offset], length);

    return (long)ret;
}

long sys_munmap(unsigned long addr, size_t len) {
    return 0;
}

long sys_mremap(unsigned long addr, unsigned long old_len, unsigned long new_len, unsigned long flags, unsigned long new_addr) {
    return 0;
}

long sys_utimensat(int dfd, const char *filename, kernel_timespec_t *utimes, int flags) {
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
