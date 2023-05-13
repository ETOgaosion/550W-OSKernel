#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <mthread.h>
#include <time.h>
#include <stdlib.h>
#include <mailbox.h>

#define SHARED_MEM 1
#define SHARED_TIME 6
#define MAX_RECV_CNT 64
#define LOCK_KEY 10

char recv_buffer[MAX_RECV_CNT * sizeof(EthernetFrame)];
char mbox_name[10]="btw";
int recv_length[MAX_RECV_CNT];

int lock_id;
int net_lock=0;
int send_lock=1;
int cat1=0, cat2=0;

void clear(int i)
{
    sys_move_cursor(1, i);
    printf("                                                                       ");
    sys_move_cursor(1, i);
}

void lock(int *l)
{
    while(atomic_exchange(l, 1))
        ;
}

void unlock(int *l)
{
    atomic_exchange(l, 0);
}

void readnet(int func)
{
    int num=0;
    sys_net_irq_mode(1);
    while(1)
    {
        lock(&net_lock);
        again:
        recv_length[0]=0;
        clear(1);
        printf("[FATHER TASK (%d)] start recv ...", num++);
        sys_net_recv(recv_buffer, sizeof(EthernetFrame), 1, recv_length);
        uint64_t* time_addr = (uint64_t*)sys_shmpageget(SHARED_TIME);
        *time_addr=sys_get_tick();
        if(recv_length[0]!=70) goto again;
        unlock(&send_lock);
    }
}

void shmcopy(int func)
{
    int num=0;
    while(1)
    {
        lock(&send_lock);
        if((uint32_t)recv_buffer[36]==0xc3 && (uint32_t)recv_buffer[37]==0x51)
        {
            clear(2);
            printf("[FATHER TASK (%d)]: port 50001 : shared memory ... ", num++);
            
            int* mem_addr = (int*)sys_shmpageget(SHARED_MEM);
            binsemop(lock_id, 0);

            *mem_addr=recv_length[0];
            mem_addr++;
            char* mem_char=(char*)mem_addr;
            char *curr = recv_buffer;
            
            for(int i=0;i<recv_length[0];i++)
            {
                *mem_char=*curr;
                mem_char++;
                curr++;
            }
            printf("done\n");
            binsemop(lock_id, 1);
            unlock(&net_lock);
            recv_buffer[36]=0;
        }
        else
            unlock(&send_lock);
    }
}

void mbcopy(int func)
{
    int num=0;
    mailbox_t mq=mbox_open(mbox_name);
    while(1)
    {
        lock(&send_lock);
        if((uint32_t)recv_buffer[36]==0xe5 && (uint32_t)recv_buffer[37]==0x40)
        {
            clear(3);
            printf("[FATHER TASK (%d)]: port 58688 : mailbox ... ", num++);
            mbox_send(mq, recv_length, 4);
            mbox_send(mq, recv_buffer, recv_length[0]);
            printf("done\n");
            recv_buffer[36]=0;
            unlock(&net_lock);
        }
        else
            unlock(&send_lock);
    }
}

char write_buffer[MAX_RECV_CNT * sizeof(EthernetFrame)];
int write_length[MAX_RECV_CNT];
int child_lock_recv1=0;
int child_lock_recv2=0;
int child_lock_write=1;

void shmrecv(int func)
{
    int num=0;
    while(1)
    {
        lock(&child_lock_recv1);

        clear(4);
        printf("[CHILD TASK (%d)]: port 50001 : shared memory ... ", num++);
        
        int* mem_addr = (int*)sys_shmpageget(SHARED_MEM);
        binsemop(lock_id, 0);
        while(*mem_addr==0)
        {
            binsemop(lock_id, 1);
            binsemop(lock_id, 0);
        }

        write_length[0]=*mem_addr;
        mem_addr++;
        char* mem_char=(char*)mem_addr;
        
        mem_addr--;
        *mem_addr=0;

        char *curr = write_buffer;
        
        for(int i=0;i<write_length[0];i++)
        {
            *curr=*mem_char;
            mem_char++;
            curr++;
        }
        printf("done\n");
        binsemop(lock_id, 1);
        unlock(&child_lock_write);
    }
}

void mbrecv(int func)
{
    int num=0;
    mailbox_t mq=mbox_open(mbox_name);
    write_length[0]=0;
    write_buffer[0]='0';
        
    while(1)
    {
        lock(&child_lock_recv2);

        clear(5);
        printf("[CHILD TASK (%d)]: port 58688 : mailbox ... ", num++);
        mbox_recv(mq, write_length, 4);    
        mbox_recv(mq, write_buffer, write_length[0]);
        printf("done\n");

        unlock(&child_lock_write);
    }
}

void writefile(int func)
{
    int num=0;
    int fd = sys_fopen("all.txt", O_RDWR);
    while(1)
    {
        lock(&child_lock_write);

        clear(6);
        printf("[CHILD TASK (%d)]: write to file ... ", num++);
        char *curr = write_buffer;
        
        for (int j = 0; j < (write_length[0] + 15) / 16; ++j) {
            for (int k = 0; k < 16 && (j * 16 + k < write_length[0]); ++k) {
                sys_fwrite(fd, curr, 1);
                ++curr;
            }
        }

        uint64_t time_thread;
        uint64_t* time_addr = (uint64_t*)sys_shmpageget(SHARED_TIME);
        time_thread=*time_addr;
        time_thread=sys_get_tick() - time_thread;
        printf("done : %d ticks", time_thread);

        unlock(&child_lock_recv1);
        unlock(&child_lock_recv2);
    }
}

int main()
{
    int id=-1;

    //ask for shared memory
    int* mem_addr = (int*)sys_shmpageget(SHARED_MEM);
    *mem_addr=0;
    uint64_t* time_addr = (uint64_t*)sys_shmpageget(SHARED_TIME);
    *time_addr=0;

    sys_touch("all.txt");

    lock_id=binsemget(LOCK_KEY);
    id = sys_fork();
    
    if(id!=0)
    {
        //father process
        mthread_t id1, id2, id3;
        mthread_create(&id1, readnet, 0);
        mthread_create(&id2, shmcopy, 0);
        mthread_create(&id3, mbcopy, 0);
        mthread_join(id1);
        mthread_join(id2);
        mthread_join(id3);
    }
    else
    {
        mthread_t id1, id2, id3;
        mthread_create(&id1, shmrecv, 0);
        mthread_create(&id2, mbrecv, 0);
        mthread_create(&id3, writefile, 0);
        mthread_join(id1);
        mthread_join(id2);
        mthread_join(id3);
    }
    return 0;
}
