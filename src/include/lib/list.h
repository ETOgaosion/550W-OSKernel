#pragma once
#include <common/type.h>

// double-linked list
//   TODO: use your own list design!!
typedef struct list_node {
    pid_t pid;
    pid_t tid;
    uint64_t time;
    uint64_t start_time;
    int num_packet;
    struct list_node *next, *prev;
} list_node_t;

typedef list_node_t list_head;
