#include <mailbox.h>
#include <string.h>
#include <os/syscall_number.h>
#include <sys/syscall.h>

mailbox_t mbox_open(char *name)
{
    // TODO:
    return invoke_syscall(SYSCALL_MBOX_OPEN, name, IGNORE, IGNORE, IGNORE);
}

void mbox_close(mailbox_t mailbox)
{
    // TODO:
    invoke_syscall(SYSCALL_MBOX_CLOSE, mailbox, IGNORE, IGNORE, IGNORE);
}

int mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    return invoke_syscall(SYSCALL_MBOX_SEND, mailbox, msg, msg_length, IGNORE);
}

int mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    return invoke_syscall(SYSCALL_MBOX_RECV, mailbox, msg, msg_length, IGNORE);
}

int test_send(mailbox_t mailbox, int msg_length)
{
    return invoke_syscall(SYSCALL_TEST_SEND, mailbox, msg_length, IGNORE, IGNORE);
}

int test_recv(mailbox_t mailbox, int msg_length)
{
    return invoke_syscall(SYSCALL_TEST_RECV, mailbox, msg_length, IGNORE, IGNORE);
}
