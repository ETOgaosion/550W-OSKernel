#include <fs/virtf.h>
#include <os/irq.h>
#include <lib/stdio.h>

int k_num2str(char *buf,int num){
    int i = 0,j;
    char str[32];
    if(num == 0){
        buf[0] = '0';
        return 1;
    }
    while(num > 0){
        str[i++]= num%10 + '0';
        num = num/10;
    }
    for(j=0;j<i;j++){
        buf[j] = str[i-j-1];
    }
    return i;
}

int k_gen_int_line(char *buf,int a,int b){
    // if(b==0)
    //     return 0;
    int len = 0;
    len+=k_num2str(&buf[len],a);
    buf[len++]=':';
    buf[len++]='\t';
    len+=k_num2str(&buf[len],b);
    buf[len++]='\n';
    return len;
}

void k_gen_int(fd_t *file){
    char *buf = (char *)k_mm_alloc_page(1);
    k_memset(buf,0,PAGE_SIZE);
    int len = 0;
    for(int i = 0 ;i <PLIC_NR_IRQS;i++){
        len += k_gen_int_line(&buf[len],i,plic_cnt[i]);
    }
    buf[len]='\0';
    ((inode_t *)file->inode)->i_mapping = (ptr_t)buf;
    file->pos = 0;
    file->size = len;
}

void k_gen_virt(fd_t *file){
    if(!k_strcmp(file->name,"interrupts"))
        k_gen_int(file);
}