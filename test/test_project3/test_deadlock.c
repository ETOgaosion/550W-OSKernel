#include <time.h>
#include "test3.h"
#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>

mailbox_t mails[5];
static const char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-={};|[],./<>?!@#$%^&*";
void test_dead(int);
void test_deadlock()
{
    mails[1] = mbox_open("test-1");
    mails[2] = mbox_open("test-2");
    mails[3] = mbox_open("test-3");
    struct task_info childtask = {(uintptr_t)&test_dead,
                                   USER_PROCESS};
    pid_t pids[5];
    for (int i = 0; i < 3; ++i) {
        pids[i] = sys_spawn(&childtask, (void*)(i+2),
                            ENTER_ZOMBIE_ON_EXIT);
    }
    for (int i = 0; i < 3; ++i) {
        sys_waitpid(pids[i]);
    }
    sys_move_cursor(1,1);
    printf("[TASK] test deadlock finished\n");
    sys_exit();
}

void test_dead(int print_location)
{
    srand(clock());
    int testsum = 0;
    for(int i=0;i<1000;i++)
    {
        int towhat = rand()%3 + 1;
        int dowhat = rand()%2;
        mailbox_t mp = mails[towhat];
        int testlen = rand()%20 + 1;
        char s[30];
        //send
        testsum+=testlen;
        if(dowhat==0)
        {
            for(int j=0;j<testlen;j++)
            {
                s[j]=alpha[rand()%80];
            }
            if(test_send(mp, testlen))
            {
                mbox_send(mp, s, testlen);
            }
            else
            {
                mbox_recv(mp, s, testlen);
            }
            sys_move_cursor(1, print_location);
            printf("[send task] succeed %d chars (%d)\n",testsum, i);
        }
        else if(dowhat==1)
        {
            if(test_recv(mp, testlen))
            {
                mbox_recv(mp, s, testlen);
            }
            else
            {
                mbox_send(mp, s, testlen);
            }
            sys_move_cursor(1, print_location);
            printf("[recv task] succeed %d chars (%d)\n",testsum, i);
        }
    }
    sys_exit();
}