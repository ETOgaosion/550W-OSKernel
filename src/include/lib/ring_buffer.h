#pragma once

#include <common/types.h>

#define RING_BUFFER_SIZE 4095

#pragma pack(8)
typedef struct ring_buffer {
    int lock;     /* FOR NOW no use */
    size_t size;  // for future use
    int32_t head; // read from head
    int32_t tail; // write from tail
    // char buf[RING_BUFFER_SIZE + 1]; // left 1 byte
    char *buf;
} ring_buffer_t;
#pragma pack()

int k_ring_buffer_wait_read(ring_buffer_t *rbuf, time_t final_ticks);
int k_ring_buffer_wait_write(ring_buffer_t *rbuf, time_t final_ticks);

void k_ring_buffer_init(ring_buffer_t *rbuf);

int k_ring_buffer_used(ring_buffer_t *rbuf);

int k_ring_buffer_free(ring_buffer_t *rbuf);

int k_ring_buffer_empty(ring_buffer_t *rbuf);

int k_ring_buffer_full(ring_buffer_t *rbuf);

size_t k_ring_buffer_read(ring_buffer_t *rbuf, char *buf, size_t size);

// rbuf should have enough space for buf
size_t k_ring_buffer_write(ring_buffer_t *rbuf, char *buf, size_t size);