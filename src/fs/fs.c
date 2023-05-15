/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <asm/common.h>
#include <asm/csr.h>
#include <asm/sbi.h>
#include <asm/syscall.h>
#include <asm/pgtable.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/lock.h>
#include <common/elf.h>
#include <lib/stdio.h>
#include <lib/time.h>
#include <lib/assert.h>
#include <drivers/screen.h>

#define O_RDONLY 1 /* read only open */
#define O_WRONLY 2 /* write only open */
#define O_RDWR 3 /* read/write open */

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

super_block_t superblock;
super_block_t superblock2;

void cpy_sb(int func, super_block_t* sb)
{
    if(func==1)
    {
        superblock.magic=sb->magic;
        superblock.fs_start_sec=sb->fs_start_sec;
        superblock.size=sb->size;
        superblock.imap_sec_offset=sb->imap_sec_offset;
        superblock.imap_sec_size=sb->imap_sec_size;
        superblock.smap_sec_offset=sb->smap_sec_offset;
        superblock.smap_sec_size=sb->smap_sec_size;
        superblock.inode_sec_offset=sb->inode_sec_offset;
        superblock.inode_sec_size=sb->inode_sec_size;
        superblock.data_sec_offset=sb->data_sec_offset;
        superblock.data_sec_size=sb->data_sec_size;
        superblock.sector_used=sb->sector_used;
        superblock.inode_used=sb->inode_used;
        superblock.root_inode_offset=sb->root_inode_offset;
    }
    else if(func==2)
    {
        superblock2.magic=superblock.magic;
        superblock2.fs_start_sec=superblock.fs_start_sec;
        superblock2.size=superblock.size;
        superblock2.imap_sec_offset=superblock.imap_sec_offset;
        superblock2.imap_sec_size=superblock.imap_sec_size;
        superblock2.smap_sec_offset=superblock.smap_sec_offset;
        superblock2.smap_sec_size=superblock.smap_sec_size;
        superblock2.inode_sec_offset=superblock.inode_sec_offset;
        superblock2.inode_sec_size=superblock.inode_sec_size;
        superblock2.data_sec_offset=superblock.data_sec_offset;
        superblock2.data_sec_size=superblock.data_sec_size;
        superblock2.sector_used=superblock.sector_used;
        superblock2.inode_used=superblock.inode_used;
        superblock2.root_inode_offset=superblock.root_inode_offset;
    }
    else if(func==3)
    {
        sb->magic=superblock.magic;
        sb->fs_start_sec=superblock.fs_start_sec;
        sb->size=superblock.size;
        sb->imap_sec_offset=superblock.imap_sec_offset;
        sb->imap_sec_size=superblock.imap_sec_size;
        sb->smap_sec_offset=superblock.smap_sec_offset;
        sb->smap_sec_size=superblock.smap_sec_size;
        sb->inode_sec_offset=superblock.inode_sec_offset;
        sb->inode_sec_size=superblock.inode_sec_size;
        sb->data_sec_offset=superblock.data_sec_offset;
        sb->data_sec_size=superblock.data_sec_size;
        sb->sector_used=superblock.sector_used;
        sb->inode_used=superblock.inode_used;
        sb->root_inode_offset=superblock.root_inode_offset;
    }
}

int fs_start_sec=114514;
int magic_number = 0x114514;
int sb_sec_offset=0;
int sb2_sec_offset=1024*1024+1;

uint64_t iab_map_addr_offset=0;
uint64_t sec_map_addr_offset=512;
uint64_t iab_map_addr_size=257*512;

uint64_t inode_addr_offset=258*512;
uint64_t dir_addr_offset=770*512;
uint64_t dir2_addr_offset=778*512;
uint64_t dir3_addr_offset=786*512;
uint64_t dir4_addr_offset=794*512;
uint64_t data_addr_offset=802*512;
uint64_t empty_block=FS_KERNEL_ADDR-0x2000;

inode_t nowinode;
int nowinodeid;

fentry_t fd[20];
int nowfid=0;
int freefid[20];
int freenum=0;

int do_touch(char* name);

int do_lseek(int fid, int offset, int whence)
{
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=fd[fid].inodeid;
    int pblock=fd[fid].pos_block;
    int poffset=fd[fid].pos_offset;
    int finaloff=0;
    
    switch (whence)
    {
    case SEEK_SET:
        fd[fid].pos_block=offset/4096;
        fd[fid].pos_offset=offset%4096;
        break;
    case SEEK_CUR:
        finaloff=pblock*4096+poffset+offset;
        fd[fid].pos_block=finaloff/4096;
        fd[fid].pos_offset=finaloff%4096;
        break;
    case SEEK_END:
        finaloff=inode->sec_size*512+offset;
        fd[fid].pos_block=finaloff/4096;
        fd[fid].pos_offset=finaloff%4096;
        break;
    default:
        return 0;
        break;
    }
    if(fd[fid].pos_block>=inode->sec_size*8)
    {
        inode->sec_size=fd[fid].pos_block*8;
    }
    return 1;
}

int alloc_fid()
{
    int id=0;
    if(freenum>0)
    {
        id=freefid[freenum];
        freenum--;
    }
    else
        id = nowfid++;
    return id;
}

void do_fclose(int fid)
{
    freenum++;
    freefid[freenum]=fid;
}

int alloc_inode()
{
    //sbi_sd_read(kva2pa(FS_KERNEL_ADDR+iab_map_addr_offset),1,
    //    superblock.imap_sec_offset+superblock.fs_start_sec);
    uint64_t* imap=(uint64_t*)(FS_KERNEL_ADDR+iab_map_addr_offset);
    int id=0;
    int inodeid=0;
    for(int i=0;i<superblock.imap_sec_size*512*8;i++)
    {
        if(!(imap[id] & (1<<(i%64))))
        {
            imap[id]=imap[id]|(1<<(i%64));
            inodeid=i;
            superblock.inode_used++;
            if((((superblock.inode_used-1)*sizeof(inode_t))%512)
                +sizeof(inode_t)>512||
                (((superblock.inode_used-1)*sizeof(inode_t))%512)==0)
                superblock.sector_used+=1;
            break;
        }
        if((i%64)==63)
            id++;
    }
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+iab_map_addr_offset),1,
        superblock.imap_sec_offset+superblock.fs_start_sec);
    sbi_sd_write(kva2pa(&superblock),
        1, fs_start_sec+sb_sec_offset);
    sbi_sd_write(kva2pa(&superblock),
        1, fs_start_sec+sb2_sec_offset);
    return inodeid;
}

int alloc_block(inode_t* inode)
{
    uint64_t* smap=(uint64_t*)(FS_KERNEL_ADDR+sec_map_addr_offset);
    int id=0;
    int sectorid=0;
    for(int i=0;i<superblock.smap_sec_size*512*8;i+=8)
    {
        if(!(smap[id] & ((uint64_t)1<<(i%64))))
        {
            smap[id]=smap[id]|((uint64_t)0xff<<(i%64));
            sectorid=i;
            superblock.sector_used+=8;
            break;
        }
        if((i%64)==56)
            id++;
    }
    inode->sec_size+=8;
    sbi_sd_write(kva2pa(empty_block),
        8, superblock.data_sec_offset+superblock.fs_start_sec
        +sectorid);
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+sec_map_addr_offset),
        superblock.smap_sec_size,
        superblock.smap_sec_offset+superblock.fs_start_sec);
    sbi_sd_write(kva2pa(&superblock),
        1, fs_start_sec+sb_sec_offset);
    sbi_sd_write(kva2pa(&superblock),
        1, fs_start_sec+sb2_sec_offset);
    return sectorid;
}

int look_up_1dir(char* name)
{
    //return inodeid if founded, otherwise -1
    int flag=0;
    int dinodeid=-1;

    dentry_t* dir=(dentry_t*)(FS_KERNEL_ADDR+dir_addr_offset);
    while(!(dir->last))
    {
        if(strcmp(dir->name,name)==0)
        {
            flag=1;
            break;
        }    
        dir++;
    }
    if(strcmp(dir->name,name)==0)
        flag=1;
    if(flag==0) return -1;
    
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=dir->inode_id;
    dinodeid=dir->inode_id;
    if(inode->mode==0)
    {
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
            inode->sec_size,
            superblock.data_sec_offset+superblock.fs_start_sec
            +inode->direct_block_pointers[0]);
    }
    return dinodeid;
}

int look_up_dir(char* buff)
{
    //if find and everything is ok, return the inodeid
    //else return -1
    char name[20];
    int i=0;
    int inodeid=nowinodeid;
    int num=kstrlen(buff);

    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    inode_t* inode = &nowinode;
    
    while(i<num)
    {
        int j = 0;
        for(i; i<num && buff[i]!=' ' && buff[i]!='/'; i++)
        {
            name[j] = buff[i];
            j++;
        }
        name[j] = '\0';
        i++;

        //file has no subdir
        if(inode->mode==1)
            return -1;
        inodeid=look_up_1dir(name);

        //no file/dir matched
        if(inodeid==-1)
            return -1;
        inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
        inode+=inodeid;
    }
    return inodeid;
}

int seek_pos(int rpos_block, int rpos_offset, inode_t* inode, int mode)
{
    int final_offset=0;
    if(rpos_block<11)
    {
        int point_num=rpos_block;
        if(inode->direct_block_pointers[point_num]==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
            {
                inode->direct_block_pointers[point_num]=alloc_block(inode);
            }
        }
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+data_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +inode->direct_block_pointers[point_num]);
        final_offset=inode->direct_block_pointers[point_num];
    }
    else if(rpos_block<1024*3+11)
    {
        int point2_num=(rpos_block-11)/1024;
        int point1_num=(rpos_block-11)%1024;
        
        if(inode->indirect_block_pointers[point2_num]==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
                inode->indirect_block_pointers[point2_num]=alloc_block(inode);
        }
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +inode->indirect_block_pointers[point2_num]);
        int* p=(int*)(FS_KERNEL_ADDR+dir_addr_offset);
        p+=point1_num;

        if(*p==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
            {
                *p=alloc_block(inode);
                sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
                    8, superblock.data_sec_offset+superblock.fs_start_sec
                    +inode->indirect_block_pointers[point2_num]);
            }
        }
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+data_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +(*p));
        final_offset=*p;
    }
    else if(rpos_block<2*1024*1024+1024*3+11)
    {
        int point3_num=(rpos_block-11-1024*3)/(1024*1024);
        int point2_num=((rpos_block-11-1024*3)%(1024*1024))/1024;
        int point1_num=(rpos_block-11-1024*3)%1024;
        
        if(inode->double_block_pointers[point3_num]==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
                inode->double_block_pointers[point3_num]=alloc_block(inode);
        }
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +inode->double_block_pointers[point3_num]);
        int* p=(int*)(FS_KERNEL_ADDR+dir_addr_offset);
        p+=point2_num;
        
        if(*p==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
            {
                *p=alloc_block(inode);
                sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
                    8, superblock.data_sec_offset+superblock.fs_start_sec
                    +inode->double_block_pointers[point3_num]);
            }
        }

        int of2=*p;
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +of2);
        p=(int*)(FS_KERNEL_ADDR+dir_addr_offset);
        p+=point1_num;

        if(*p==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
            {
                *p=alloc_block(inode);
                sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
                    8, superblock.data_sec_offset+superblock.fs_start_sec
                    +of2);
            }
        }

        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+data_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +*p);
        final_offset=*p;
    }
    else if(rpos_block<1024*1024*1024+2*1024*1024+1024*3+11)
    {
        int point3_num=(rpos_block-11-1024*3-2*1024*1024)/(1024*1024);
        int point2_num=(rpos_block-11-1024*3-2*1024*1024)%(1024*1024)/1024;
        int point1_num=(rpos_block-11-1024*3-2*1024*1024)%1024;
        
        if(inode->trible_block_pointers==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
                inode->trible_block_pointers=alloc_block(inode);
        }
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +inode->trible_block_pointers);
        int* p=(int*)(FS_KERNEL_ADDR+dir_addr_offset);
        p+=point3_num;

        if(*p==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
            {
                *p=alloc_block(inode);
                sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
                    8, superblock.data_sec_offset+superblock.fs_start_sec
                    +inode->trible_block_pointers);
            }
        }

        int of2=*p;
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +of2);
        p=(int*)(FS_KERNEL_ADDR+dir_addr_offset);
        p+=point2_num;

        if(*p==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
            {
                *p=alloc_block(inode);
                sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
                    8, superblock.data_sec_offset+superblock.fs_start_sec
                    +of2);
            }
        }

        int of1=*p;
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +of1);
        p=(int*)(FS_KERNEL_ADDR+dir_addr_offset);
        p+=point1_num;

        if(*p==0)
        {
            if(mode==0)
                return -1;
            else if(mode==1)
            {
                *p=alloc_block(inode);
                sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
                    8, superblock.data_sec_offset+superblock.fs_start_sec
                    +of1);
            }
        }
        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+data_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +*p);
        final_offset=*p;
    }
    return final_offset;
}

int do_fopen(char* name, int access)
{
    int flag=0;
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    dentry_t* dir=(dentry_t*)(FS_KERNEL_ADDR+dir_addr_offset);
    while(!(dir->last))
    {
        if(strcmp(dir->name,name)==0&&dir->mode==1)
        {
            flag=1;
            break;
        }    
        dir++;
    }
    if(strcmp(dir->name,name)==0&&dir->mode==1)
        flag=1;
    if(flag==0)
    {
        if(access==-1)
            return -1;
        do_touch(name);
        dir++;
    }
    int fid=alloc_fid();
    fd[fid].inodeid=dir->inode_id;
    fd[fid].prive=access;
    fd[fid].pos_block=0;
    fd[fid].pos_offset=0;
    return fid;
}
int fread(int fid, unsigned char *buff, int size)
{
    if(fd[fid].prive==O_WRONLY)
        return -1;
    int rpos_block=fd[fid].pos_block;
    int rpos_offset=fd[fid].pos_offset;

    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=fd[fid].inodeid;
    
    int nowsize=0;
    unsigned char* src;
    while(nowsize<size)
    {
        int offset=rpos_offset;
        int flag=seek_pos(rpos_block, rpos_offset, inode, 0);
        if(flag==0||flag==-1) return -1;
        src=(char*)(FS_KERNEL_ADDR+data_addr_offset+offset);
        while(rpos_offset<512*8&&nowsize<size)
        {
            *(buff++)=*(src++);
            nowsize++;
            rpos_offset++;
        }
        if(rpos_offset>=512*8)
        {
            rpos_offset=rpos_offset%(512*8);
            rpos_block++;
        }
    }
    fd[fid].pos_block=rpos_block;
    fd[fid].pos_offset=rpos_offset;
    return nowsize;
}

unsigned char test_elf[512*200];
int file_len=0;

int try_get_from_file(const char *file_name, unsigned char **binary, int *length)
{
    int fid=do_fopen(file_name, -1);
    if(fid==-1) return 0;
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=fd[fid].inodeid;
    
    file_len=inode->sec_size*512;
    fread(fid, test_elf, file_len);
    
    do_fclose(fid);
    *binary=test_elf;
    *length=file_len;
    return 1;
}

int do_fread(int fid, char *buff, int size)
{
    if(fd[fid].prive==O_WRONLY)
        return -1;
    int rpos_block=fd[fid].pos_block;
    int rpos_offset=fd[fid].pos_offset;

    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=fd[fid].inodeid;
    
    int nowsize=0;
    char* src;
    while(nowsize<size)
    {
        int offset=rpos_offset;
        int flag=seek_pos(rpos_block, rpos_offset, inode, 0);
        if(flag==0||flag==-1) return -1;
        src=(char*)(FS_KERNEL_ADDR+data_addr_offset+offset);
        while(rpos_offset<512*8&&nowsize<size)
        {
            *(buff++)=*(src++);
            nowsize++;
            rpos_offset++;
        }
        if(rpos_offset>=512*8)
        {
            rpos_offset=rpos_offset%(512*8);
            rpos_block++;
        }
    }
    fd[fid].pos_block=rpos_block;
    fd[fid].pos_offset=rpos_offset;
    return nowsize;
}

int do_fwrite(int fid, char *buff, int size)
{
    if(fd[fid].prive==O_RDONLY)
        return -1;
    int rpos_block=fd[fid].pos_block;
    int rpos_offset=fd[fid].pos_offset;

    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=fd[fid].inodeid;

    int nowsize=0;
    char* dest;
    while(nowsize<size)
    {
        int offset=rpos_offset;
        int data_off=seek_pos(rpos_block, rpos_offset, inode, 1);
        dest=(char*)(FS_KERNEL_ADDR+data_addr_offset+offset);
        while(rpos_offset<512*8&&nowsize<size)
        {
            *(dest++)=*(buff++);
            nowsize++;
            rpos_offset++;
        }
        if(rpos_offset>=512*8)
        {
            rpos_offset=rpos_offset%(512*8);
            rpos_block++;
        }
        sbi_sd_write(kva2pa(FS_KERNEL_ADDR+data_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +data_off);
    }
    fd[fid].pos_block=rpos_block;
    fd[fid].pos_offset=rpos_offset;
    
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+inode_addr_offset),
        superblock.inode_sec_size, 
        superblock.fs_start_sec+superblock.inode_sec_offset);
    return nowsize;
}

void init_dir(int inodeid)
{
    
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=inodeid;
    inode->sec_size=0;
    int block_sec_offset=alloc_block(inode);
    
    inode->mode=0;  //0-directory, 1-file
    inode->link_num=0;
    inode->direct_block_pointers[0]=block_sec_offset;
    
    dentry_t* dentry=(dentry_t*)(FS_KERNEL_ADDR+data_addr_offset);
    dentry->inode_id=inodeid;
    kstrcpy(dentry->name,".");
    dentry->last=0;
    dentry++;
    dentry->inode_id=nowinodeid;
    kstrcpy(dentry->name,"..");
    dentry->last=1;

    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+inode_addr_offset),
        superblock.inode_sec_size, 
        superblock.fs_start_sec+superblock.inode_sec_offset);
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+data_addr_offset),
        1, superblock.fs_start_sec+superblock.data_sec_offset
        +block_sec_offset);
}

void init_file(int inodeid)
{
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=inodeid;
    inode->sec_size=0;
    inode->mode=1;  //0-directory, 1-file
    inode->link_num=0;
    for(int i=0;i<11;i++)
        inode->direct_block_pointers[i]=0;
    for(int i=0;i<3;i++)
        inode->indirect_block_pointers[i]=0;
    for(int i=0;i<2;i++)
        inode->double_block_pointers[i]=0;
    inode->trible_block_pointers=0;

    //no write, no need for data block

    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+inode_addr_offset),
        superblock.inode_sec_size, 
        superblock.fs_start_sec+superblock.inode_sec_offset);
}

int do_mkdir(char* name)
{
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    dentry_t* dir=(dentry_t*)(FS_KERNEL_ADDR+dir_addr_offset);
    while(!(dir->last))
    {
        if(strcmp(dir->name,name)==0)
            return 0;
        dir++;
    }
    if(strcmp(dir->name,name)==0)
        return 0;
    dir->last=0;
    dir++;
    dir->inode_id=alloc_inode();
    init_dir(dir->inode_id);
    dir->mode=0;
    dir->last=1;
    kstrcpy(dir->name,name);
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    return 1;
}

int do_touch(char* name)
{
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    dentry_t* dir=(dentry_t*)(FS_KERNEL_ADDR+dir_addr_offset);
    while(!(dir->last))
    {
        if(strcmp(dir->name,name)==0)
            return 0;
        dir++;
    }
    if(strcmp(dir->name,name)==0)
        return 0;
    dir->last=0;
    dir++;
    dir->inode_id=alloc_inode();
    init_file(dir->inode_id);
    dir->mode=1;
    dir->last=1;
    kstrcpy(dir->name,name);
    
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    return 1;
}

//extern vt100_clear();
int do_ln(char* name, char *path)
{
    //vt100_clear();
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir2_addr_offset),
        8, superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    dentry_t* dir=(dentry_t*)(FS_KERNEL_ADDR+dir2_addr_offset);
    while(!(dir->last))
    {
        if(strcmp(dir->name,name)==0)
            return 0;
        dir++;
    }
    if(strcmp(dir->name,name)==0)
        return 0;
    int inodeid = look_up_dir(path);
    if(inodeid==-1) return 0;
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=inodeid;
    if(inode->mode==0) return 0;
    inode->link_num++;

    dir->last=0;
    dir++;
    dir->inode_id=inodeid;
    dir->mode=1;
    dir->last=1;
    kstrcpy(dir->name,name);
    
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir2_addr_offset),
        8, superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    return 1;
}

int do_rmdir(char *name)
{
    int flag=0;
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    dentry_t* dir=(dentry_t*)(FS_KERNEL_ADDR+dir_addr_offset);
    int dinode;
    while(!(dir->last))
    {
        if(strcmp(dir->name,name)==0)
        {
            flag=1;
            break;
        }    
        dir++;
    }
    if(strcmp(dir->name,name)==0)
        flag=1;
    if(strcmp(name, ".")==0||strcmp(name, "..")==0)
        flag=0;
    if(flag==0) return 0;
    if(dir->mode==1) return 0; //file not dir
    dinode=dir->inode_id;
    
    if(dir->last)
    {
        dir--;
        dir->last=1;
        dir++;
    }
    else
    {
        dentry_t* p=dir;
        while(!(p->last))
        {
            p->inode_id=(p+1)->inode_id;
            p->last=(p+1)->last;
            p->mode=(p+1)->mode;
            kstrcpy(p->name, (p+1)->name);
            p++;
        }
    }

    uint64_t* imap=(uint64_t*)(FS_KERNEL_ADDR+iab_map_addr_offset);
    int id=dinode/64;
    int off=dinode%64;
    imap[id] &= ~(((uint64_t)1)<<off);

    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+iab_map_addr_offset),1,
        superblock.imap_sec_offset+superblock.fs_start_sec);
    
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode += dinode;
    
    int data_sec_start=inode->direct_block_pointers[0];

    uint64_t* smap=(uint64_t*)(FS_KERNEL_ADDR+sec_map_addr_offset);
    id=data_sec_start/64;
    off=data_sec_start%64;
    smap[id] &= ~(((uint64_t)0xff)<<off);

    superblock.inode_used--;
    superblock.sector_used-=inode->sec_size;

    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+sec_map_addr_offset),
        superblock.smap_sec_size,
        superblock.smap_sec_offset+superblock.fs_start_sec);
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    return 1;
}

void getback_1block(int data_sec_start, uint64_t* smap, int* nowsec)
{
    int id, off;
    id=data_sec_start/64;
    off=data_sec_start%64;
    smap[id] &= ~(((uint64_t)0xff)<<off);
    (*nowsec)+=8;
}

int getback_block(inode_t* inode)
{
    int nowsec=0;
    int id, off, data_sec_start;
    int pointer1_sec_start, pointer2_sec_start, pointer3_sec_start;
    uint64_t* smap=(uint64_t*)(FS_KERNEL_ADDR+sec_map_addr_offset);
    for(int i=0;i<11;i++)
    {
        data_sec_start=inode->direct_block_pointers[i];
        if(data_sec_start!=0)
        {
            getback_1block(data_sec_start, smap, &nowsec);
        }   
    }
    if(nowsec>=inode->sec_size) return nowsec;
    for(int i=0;i<3;i++)
    {
        pointer1_sec_start=inode->indirect_block_pointers[i];
        if(pointer1_sec_start!=0)
        {
            getback_1block(pointer1_sec_start, smap, &nowsec);

            sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir2_addr_offset),
                8, superblock.data_sec_offset+superblock.fs_start_sec
                +inode->indirect_block_pointers[i]);
            int *p=(int*)(FS_KERNEL_ADDR+dir2_addr_offset);
            for(int j=0;j<1024;j++)
            {
                if(p[j]!=0)
                {
                    getback_1block(p[j], smap, &nowsec);
                }
                if(nowsec>=inode->sec_size) return nowsec;
            }
        }
    }
    for(int i=0;i<2;i++)
    {
        pointer1_sec_start=inode->double_block_pointers[i];
        if(pointer1_sec_start!=0)
        {
            getback_1block(pointer1_sec_start, smap, &nowsec);

            sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir2_addr_offset),
                8, superblock.data_sec_offset+superblock.fs_start_sec
                +inode->double_block_pointers[i]);
            int *p=(int*)(FS_KERNEL_ADDR+dir2_addr_offset);
            for(int j=0;j<1024;j++)
            {
                if(p[j]!=0)
                {
                    getback_1block(p[j], smap, &nowsec);

                    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir3_addr_offset),
                        8, superblock.data_sec_offset+superblock.fs_start_sec
                        +p[j]);
                    int *p2=(int*)(FS_KERNEL_ADDR+dir3_addr_offset);
                    for(int k=0;k<1024;k++)
                    {
                        if(p2[k]!=0)
                        {
                            getback_1block(p2[k], smap, &nowsec);
                        }
                        if(nowsec>=inode->sec_size) return nowsec;
                    }
                }
            }
        }
    }
    if(inode->trible_block_pointers!=0)
    {
        getback_1block(inode->trible_block_pointers, smap, &nowsec);

        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir2_addr_offset),
            8, superblock.data_sec_offset+superblock.fs_start_sec
            +inode->trible_block_pointers);
        int *p=(int*)(FS_KERNEL_ADDR+dir2_addr_offset);
        for(int j=0;j<1024;j++)
        {
            if(p[j]!=0)
            {
                getback_1block(p[j], smap, &nowsec);
                sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir3_addr_offset),
                    8, superblock.data_sec_offset+superblock.fs_start_sec
                    +p[j]);
                int *p2=(int*)(FS_KERNEL_ADDR+dir3_addr_offset);
                for(int k=0;k<1024;k++)
                {
                    if(p2[k]!=0)
                    {
                        getback_1block(p2[k], smap, &nowsec);
                        sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir4_addr_offset),
                            8, superblock.data_sec_offset+superblock.fs_start_sec
                            +p2[k]);
                        int *p3=(int*)(FS_KERNEL_ADDR+dir4_addr_offset);
                        for(int m=0;m<1024;m++)
                        {
                            if(p3[m]!=0)
                            {
                                getback_1block(p3[m], smap, &nowsec);
                            }
                            if(nowsec>=inode->sec_size) return nowsec;
                        }
                    }
                }
            }
        }
    }
    return nowsec;
}

int do_rm(char *name)
{
    int flag=0;
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    dentry_t* dir=(dentry_t*)(FS_KERNEL_ADDR+dir_addr_offset);
    int dinode;
    while(!(dir->last))
    {
        if(strcmp(dir->name,name)==0)
        {
            flag=1;
            break;
        }    
        dir++;
    }
    if(strcmp(dir->name,name)==0)
        flag=1;
    if(flag==0) return 0;
    if(dir->mode==0) return 0; //dir not file
    dinode=dir->inode_id;
    if(dir->last)
    {
        dir--;
        dir->last=1;
        dir++;
    }
    else
    {
        dentry_t* p=dir;
        while(!(p->last))
        {
            p->inode_id=(p+1)->inode_id;
            p->last=(p+1)->last;
            p->mode=(p+1)->mode;
            kstrcpy(p->name, (p+1)->name);
            p++;
        }
    }

    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode += dinode;
    if(inode->link_num!=0)
    {
        inode->link_num--;
        sbi_sd_write(kva2pa(FS_KERNEL_ADDR+inode_addr_offset),
        superblock.inode_sec_size,
        superblock.inode_sec_offset+superblock.fs_start_sec);
        sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
        return 1;
    }
    
    uint64_t* imap=(uint64_t*)(FS_KERNEL_ADDR+iab_map_addr_offset);
    int id=dinode/64;
    int off=dinode%64;
    imap[id] &= ~(((uint64_t)1)<<off);

    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+iab_map_addr_offset),1,
        superblock.imap_sec_offset+superblock.fs_start_sec);

    superblock.inode_used--;
    superblock.sector_used-=getback_block(inode);

    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+sec_map_addr_offset),
        superblock.smap_sec_size,
        superblock.smap_sec_offset+superblock.fs_start_sec);
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+dir_addr_offset),
        nowinode.sec_size,
        superblock.data_sec_offset+superblock.fs_start_sec
        +nowinode.direct_block_pointers[0]);
    return 1;
}

void update_inode(inode_t* src, int id)
{
    nowinode.mode=src->mode;
    nowinode.sec_size=src->sec_size;
    for(int i=0;i<20;i++)
        nowinode.direct_block_pointers[i]=src->direct_block_pointers[i];
    for(int i=0;i<3;i++)
        nowinode.indirect_block_pointers[i]=src->indirect_block_pointers[i];
    for(int i=0;i<2;i++)
        nowinode.double_block_pointers[i]=src->double_block_pointers[i];
    nowinode.trible_block_pointers=src->trible_block_pointers;    
    nowinodeid=id;
}

int do_cd(char* name)
{
    int inodeid=0;
    inodeid=look_up_dir(name);
    if(inodeid==-1) return 0;
    
    //update nowinode
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=inodeid;
    if(inode->mode==1) return 0; //file
    update_inode(inode, inodeid);
    return 1;
}

int getwei(int a)
{
    int num=0;
    if(a==0) return 1;
    while(a)
    {
        num++;
        a/=10;
    }
    return num;
}

void printw(int num, int what)
{
    while(num--) prints(" ");
    prints("%d",what);
}

int do_ls(char* name, int func)
{
    int inodeid=look_up_dir(name);
    if(inodeid==-1) return 0;
    
    dentry_t* dir=(dentry_t*)(FS_KERNEL_ADDR+dir_addr_offset);
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=inodeid;
    if(inode->mode==1) return 0;

    int len=0;
    int num=0;
    if(func==1)
        prints("[%d] name        inode        ln           size(B)\n",num++);
    while(!(dir->last))
    {
        prints("[%d] %s",num++,dir->name);
        if(dir->mode==0)
            prints("/");
        if(func==1)
        {
            len=kstrlen(dir->name);
            if(dir->mode==0) len++;
            inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
            inode+=dir->inode_id;
            len=13-len;
            printw(len, dir->inode_id);
            len=13-getwei(dir->inode_id);
            printw(len, inode->link_num);
            len=13-getwei(inode->link_num);
            printw(len, inode->sec_size*512);
        }
        prints("\n");    
        dir++;
    }
    prints("[%d] %s",num++,dir->name);
    if(dir->mode==0)
        prints("/");
    if(func==1)
    {
        len=kstrlen(dir->name);
        if(dir->mode==0) len++;
        inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
        inode+=dir->inode_id;
        len=13-len;
        printw(len, dir->inode_id);
        len=13-getwei(dir->inode_id);
        printw(len, inode->link_num);
        len=13-getwei(inode->link_num);
        printw(len, inode->sec_size*512);
    }
    prints("\n");
    return num;    
}

int do_cat(char* name)
{
    int inodeid=look_up_dir(name);
    if(inodeid==-1) return 0;
    inode_t* inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    inode+=inodeid;
    if(inode->sec_size==0||inode->mode==0) return 0;

    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+data_addr_offset),
        8,
        superblock.data_sec_offset+superblock.fs_start_sec
        +inode->direct_block_pointers[0]);
    
    char* c=(char*)(FS_KERNEL_ADDR+data_addr_offset);
    int off=0;
    int times=1024;
    while(*c&&(times--))
    {
        if(*c=='\n') off++;
        prints("%c",*c);
        c++;
    }
    return off;
}

int do_mkfs(int func)
{
    //func: 0 not prints, 1 prints
    superblock.magic=magic_number;
    //512MB
    superblock.size=1024*1024;
    superblock.fs_start_sec=fs_start_sec;
    superblock.imap_sec_offset=1;
    superblock.imap_sec_size=1;
    superblock.smap_sec_offset=2;
    superblock.smap_sec_size=256;
    superblock.inode_sec_offset=258;
    superblock.inode_sec_size=512;
    superblock.data_sec_offset=770;
    superblock.data_sec_size=superblock.size-superblock.data_sec_offset+1;
    
    //root occupy inode[0] and data[0-7]
    uint64_t* iabmap=(uint64_t*)(FS_KERNEL_ADDR+iab_map_addr_offset);
    for(int i=0;i<iab_map_addr_size/8;i++)
    {
        if(i==0)
            iabmap[i]=1;
        else if(i==64)
            iabmap[i]=0xff;
        else
            iabmap[i]=0;
    }    
    inode_t* root_inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    root_inode->link_num=0;
    root_inode->sec_size=8;
    root_inode->mode=0;  //0-directory, 1-file
    root_inode->direct_block_pointers[0]=0;
    
    dentry_t* root_dentry=(dentry_t*)(FS_KERNEL_ADDR+data_addr_offset);
    root_dentry->inode_id=0;
    kstrcpy(root_dentry->name,".");
    root_dentry->mode=0;
    root_dentry->last=0;
    root_dentry++;
    root_dentry->inode_id=0;
    kstrcpy(root_dentry->name,"..");
    root_dentry->mode=0;
    root_dentry->last=1;

    superblock.inode_used=1;
    superblock.sector_used=266; //257+1+8
    cpy_sb(2, NULL);

    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+iab_map_addr_offset),
        257,superblock.imap_sec_offset+superblock.fs_start_sec);
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+inode_addr_offset),
        1, superblock.fs_start_sec+superblock.inode_sec_offset);
    sbi_sd_write(kva2pa(FS_KERNEL_ADDR+data_addr_offset),
        1, superblock.fs_start_sec+superblock.data_sec_offset);

    sbi_sd_write(kva2pa(&superblock),
        1, fs_start_sec+sb_sec_offset);
    sbi_sd_write(kva2pa(&superblock),
        1, fs_start_sec+sb2_sec_offset);

    if(func!=-1)
    {
        update_inode(root_inode, 0);
        prints("[FS] Start initialize filesystem!\n");
        prints("[FS] Setting superblock...\n");
        prints("     magic : 0x%lx\n",superblock.magic);
        prints("     size : %d sector, start sector : %d\n",
            superblock.size, superblock.fs_start_sec);
        prints("     inode map offset : %d (%d)\n",
            superblock.imap_sec_offset,superblock.imap_sec_size);
        prints("     sector map offset : %d (%d)\n",
            superblock.smap_sec_offset,superblock.smap_sec_size);
        prints("     inode offset : %d (%d)\n",
            superblock.inode_sec_offset,superblock.inode_sec_size);
        prints("     data offset : %d (%d)\n",
            superblock.data_sec_offset,superblock.data_sec_size);
        prints("[FS] Setting inode-map...\n");
        prints("[FS] Setting sector-map...\n");
        prints("[FS] Setting inode...\n");
        prints("[FS] Initialize filesystem finished\n");
        return 12;
    }
    return 0;
}

int do_statfs()
{
    prints("magic : 0x%lx (JFS)\n",superblock.magic);
    prints("used sector : %d/%d, start sector : %d\n",
        superblock.sector_used, superblock.size, superblock.fs_start_sec);
    prints("inode map offset  : %d, occupied sector : %d, used : %d/%d\n",
        superblock.imap_sec_offset,superblock.imap_sec_size,
        superblock.inode_used,superblock.imap_sec_size*512*8);
    prints("sector map offset : %d, occupied sector : %d\n",
        superblock.smap_sec_offset,superblock.smap_sec_size);
    prints("inode offset : %d, occupied sector : %d\n",
        superblock.inode_sec_offset,superblock.inode_sec_size);
    prints("data offset  : %d, occupied sector : %d\n",
        superblock.data_sec_offset,superblock.data_sec_size);
    prints("inode entry size : %dB, dir entry size : %dB\n",
        sizeof(inode_t),sizeof(dentry_t));
    return 7;
}

extern map_fs_space();

void init_fs()
{
    map_fs_space();
    kmemset(empty_block, 0, 0x1000);
    
    //main superblock and back-up superblock
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset), 1, fs_start_sec+sb_sec_offset);
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+dir_addr_offset+512), 1, fs_start_sec+sb2_sec_offset);
    super_block_t* sb=(super_block_t*)(FS_KERNEL_ADDR+dir_addr_offset);
    super_block_t* sb2=(super_block_t*)(FS_KERNEL_ADDR+dir_addr_offset+512);
    //not inited, then initialize
    if(sb->magic!=magic_number&&sb2->magic!=magic_number)
        do_mkfs(-1);
    //make sure one of them can work
    else if(sb->magic!=magic_number&&sb2->magic==magic_number)
        cpy_sb(1, sb2);
    else
        cpy_sb(1, sb);
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+iab_map_addr_offset),
        superblock.imap_sec_size+superblock.smap_sec_size,
        superblock.imap_sec_offset+superblock.fs_start_sec);
    sbi_sd_read(kva2pa(FS_KERNEL_ADDR+inode_addr_offset),
        superblock.inode_sec_size,
        superblock.inode_sec_offset+superblock.fs_start_sec);
    inode_t* root_inode=(inode_t*)(FS_KERNEL_ADDR+inode_addr_offset);
    update_inode(root_inode, 0);
}