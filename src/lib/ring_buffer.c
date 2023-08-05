#include <lib/math.h>
#include <lib/ring_buffer.h>
#include <lib/string.h>
#include <os/lock.h>
#include <os/pcb.h>

void k_ring_buffer_init(ring_buffer_t *rbuf) {
    // there is always one byte which should not be read or written
    k_memset(rbuf, 0, sizeof(ring_buffer_t)); /* head = tail = 0 */
    rbuf->size = RING_BUFFER_SIZE;
    // WH ADD
    // rbuf->buf = kmalloc(PAGE_SIZE);
    k_mutex_lock_init(&rbuf->lock);
    return;
}

int k_ring_buffer_used(ring_buffer_t *rbuf) {
    return (rbuf->tail - rbuf->head + rbuf->size) % (rbuf->size);
}

int k_ring_buffer_free(ring_buffer_t *rbuf) {
    // let 1 byte to distinguish empty buffer and full buffer
    return rbuf->size - k_ring_buffer_used(rbuf) - 1;
}

int k_ring_buffer_empty(ring_buffer_t *rbuf) {
    return k_ring_buffer_used(rbuf) == 0;
}

int k_ring_buffer_full(ring_buffer_t *rbuf) {
    return k_ring_buffer_free(rbuf) == 0;
}

size_t k_ring_buffer_read(ring_buffer_t *rbuf, char *buf, size_t size) {
    k_mutex_lock_acquire(rbuf->lock);
    int32_t len = min(k_ring_buffer_used(rbuf), size);
    if (len > 0) {
        if (rbuf->head + len > rbuf->size) {
            int32_t right = rbuf->size - rbuf->head, left = len - right;
            k_memcpy(buf, rbuf->buf + rbuf->head, right);
            k_memcpy(buf + right, rbuf->buf, left);
        } else {
            k_memcpy(buf, rbuf->buf + rbuf->head, len);
        }

        rbuf->head = (rbuf->head + len) % (rbuf->size);
    }
    k_mutex_lock_release(rbuf->lock);
    return len;
}

// rbuf should have enough space for buf
size_t k_ring_buffer_write(ring_buffer_t *rbuf, char *buf, size_t size) {
    k_mutex_lock_acquire(rbuf->lock);
    int32_t len = min(k_ring_buffer_free(rbuf), size);
    if (len > 0) {
        if (rbuf->tail + len > rbuf->size) {
            int32_t right = rbuf->size - rbuf->tail, left = len - right;
            k_memcpy(rbuf->buf + rbuf->tail, buf, right);
            if (left > 0) {
                k_memcpy(rbuf->buf, buf + right, left);
            }
        } else {
            k_memcpy(rbuf->buf + rbuf->tail, buf, len);
        }

        rbuf->tail = (rbuf->tail + len) % (rbuf->size);
    }
    k_mutex_lock_release(rbuf->lock);
    return len;
}

/* wait until ring buffer is not empty or timeout */
/* wait success return 0, timeout return 1, error return -1 */
int k_ring_buffer_wait_read(ring_buffer_t *rbuf, time_t final_ticks) {
    while (k_ring_buffer_empty(rbuf)) {
        time_t now_ticks = k_time_get_ticks();
        if (final_ticks < now_ticks) {
            return 1;
        }
        k_pcb_scheduler(false, false);
    }
    return 0;
}

/* wait until ring buffer is not full or timeout */
/* wait success return 0, timeout return 1, error return -1 */
int k_ring_buffer_wait_write(ring_buffer_t *rbuf, time_t final_ticks) {
    while (k_ring_buffer_full(rbuf)) {
        time_t now_ticks = k_time_get_ticks();
        if (final_ticks < now_ticks) {
            return 1;
        }
        k_pcb_scheduler(false, false);
    }
    return 0;
}