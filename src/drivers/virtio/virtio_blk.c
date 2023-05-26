#include <drivers/virtio/virtio.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/pcb.h>

disk_t disk;

void virtio_disk_init(void) {
    uint32 status = 0;

    k_spin_lock_init(&disk.vdisk_lock);

    if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 || *R(VIRTIO_MMIO_VERSION) != 2 || *R(VIRTIO_MMIO_DEVICE_ID) != 2 || *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551) {
        panic("could not find virtio disk");
    }

    // reset device
    *R(VIRTIO_MMIO_STATUS) = status;

    // set ACKNOWLEDGE status bit
    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS) = status;

    // set DRIVER status bit
    status |= VIRTIO_CONFIG_S_DRIVER;
    *R(VIRTIO_MMIO_STATUS) = status;

    // negotiate features
    uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

    // tell device that feature negotiation is complete.
    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    // re-read status to ensure FEATURES_OK is set.
    status = *R(VIRTIO_MMIO_STATUS);
    if (!(status & VIRTIO_CONFIG_S_FEATURES_OK)) {
        panic("virtio disk FEATURES_OK unset");
    }

    // initialize queue 0.
    *R(VIRTIO_MMIO_QUEUE_SEL) = 0;

    // ensure queue 0 is not in use.
    if (*R(VIRTIO_MMIO_QUEUE_READY)) {
        panic("virtio disk should not be ready");
    }

    // check maximum queue size.
    uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0) {
        panic("virtio disk has no queue 0");
    }
    if (max < DESC_NUM) {
        panic("virtio disk max queue too short");
    }

    // allocate and zero queue memory.
    k_memset((void *)disk.pages, 0, 2 * NORMAL_PAGE_SIZE);
    disk.desc = (virtq_desc_t *)disk.pages;
    disk.avail = (virtq_avail_t *)(disk.pages + DESC_NUM * sizeof(virtio_blk_req_t));
    disk.used = (virtq_used_t *)(disk.pages + NORMAL_PAGE_SIZE);

    // set queue size.
    *R(VIRTIO_MMIO_QUEUE_NUM) = DESC_NUM;

    // write physical addresses.
    *R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)disk.desc;
    *R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)disk.desc >> 32;
    *R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)disk.avail;
    *R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)disk.avail >> 32;
    *R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)disk.used;
    *R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)disk.used >> 32;

    // queue is ready.
    *R(VIRTIO_MMIO_QUEUE_READY) = 0x1;

    // all DESC_NUM descriptors start out unused.
    for (int i = 0; i < DESC_NUM; i++) {
        disk.free[i] = 1;
    }

    // tell device we're completely ready.
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    // plic.c and trap.c arrange for interrupts from VIRTIO0_IRQ.
}

// find a free descriptor, mark it non-free, return its index.
static int alloc_desc() {
    for (int i = 0; i < DESC_NUM; i++) {
        if (disk.free[i]) {
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

// mark a descriptor as free.
static void free_desc(int i) {
    if (i >= DESC_NUM) {
        panic("free_desc 1");
    }
    if (disk.free[i]) {
        panic("free_desc 2");
    }
    disk.desc[i].addr = 0;
    disk.desc[i].len = 0;
    disk.desc[i].flags = 0;
    disk.desc[i].next = 0;
    disk.free[i] = 1;
    k_wakeup(&disk.free[0]);
}

// free a chain of descriptors.
// static void free_chain(int i) {
//     while (1) {
//         int flag = disk.desc[i].flags;
//         int nxt = disk.desc[i].next;
//         free_desc(i);
//         if (flag & VRING_DESC_F_NEXT) {
//             i = nxt;
//         } else {
//             break;
//         }
//     }
// }

// allocate three descriptors (they need not be contiguous).
// disk transfers always use three descriptors.
static int alloc3_desc(int *idx) {
    for (int i = 0; i < 3; i++) {
        idx[i] = alloc_desc();
        if (idx[i] < 0) {
            for (int j = 0; j < i; j++) {
                free_desc(idx[j]);
            }
            return -1;
        }
    }
    return 0;
}

void virtio_disk_rw(buf_t *b, int write) {
    uint64 sector = b->blockno * (BSIZE / 512);

    k_spin_lock_acquire(&disk.vdisk_lock);

    // the spec's Section 5.2 says that legacy block operations use
    // three descriptors: one for type/reserved/sector, one for the
    // data, one for a 1-byte status result.

    // allocate the three descriptors.
    int idx[3];
    while (1) {
        if (alloc3_desc(idx) == 0) {
            break;
        }
        k_sleep(&disk.free[0], &disk.vdisk_lock);
    }

    // format the three descriptors.
    // qemu's virtio-blk.c reads them.

    struct virtio_blk_req *buf0 = &disk.ops[idx[0]];

    if (write) {
        buf0->type = VIRTIO_BLK_T_OUT; // write the disk
    } else {
        buf0->type = VIRTIO_BLK_T_IN; // read the disk
    }
    buf0->reserved = 0;
    buf0->sector = sector;

    disk.desc[idx[0]].addr = (uint64)buf0;
    disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (uint64)b->data;
    disk.desc[idx[1]].len = BSIZE;
    if (write) {
        disk.desc[idx[1]].flags = 0; // device reads b->data
    } else {
        disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // device writes b->data
    }
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    disk.info[idx[0]].status = 0xff; // device writes 0 on success
    disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    // record buf_t for virtio_disk_intr().
    b->disk = 1;
    disk.info[idx[0]].b = b;

    // tell the device the first index in our chain of descriptors.
    disk.avail->ring[disk.avail->idx % DESC_NUM] = idx[0];

    // tell the device another avail ring entry is available.
    disk.avail->idx += 1; // not % DESC_NUM ...

    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number

    // Wait for virtio_disk_intr() to say request has finished.
    while (b->disk == 1) {
        k_sleep(b, &disk.vdisk_lock);
    }

    disk.info[idx[0]].b = 0;

    k_spin_lock_release(&disk.vdisk_lock);
}

struct {
    spin_lock_t lock;
    buf_t buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // Sorted by how recently the buffer was used.
    // head.next is most recent, head.prev is least.
    buf_t head;
} bcache;

void binit(void) {
    buf_t *b;

    k_spin_lock_init(&bcache.lock);

    // Create linked list of buffers
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        k_sleep_lock_init(&b->lock);
        bcache.head.next->prev = b;
        bcache.head.next = b;
        b->dev = DEV_VDA2;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static buf_t *bget(uint dev, uint blockno) {
    buf_t *b;

    k_spin_lock_acquire(&bcache.lock);

    // Is the block already cached?
    for (b = bcache.head.next; b != &bcache.head; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            k_spin_lock_release(&bcache.lock);
            k_sleep_lock_aquire(&b->lock);
            return b;
        }
    }

    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if (b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            k_spin_lock_release(&bcache.lock);
            k_sleep_lock_aquire(&b->lock);
            return b;
        }
    }
    panic("bget: no buffers");
    return NULL;
}

// Return a locked buf with the contents of the indicated block.
buf_t *bread(uint dev, uint blockno) {
    buf_t *b;

    b = bget(dev, blockno);
    if (!b->valid) {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(buf_t *b) {
    if (!k_sleep_lock_hold(&b->lock)) {
        panic("bwrite");
    }
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelease(buf_t *b) {
    if (!k_sleep_lock_hold(&b->lock)) {
        panic("brelse");
    }

    k_sleep_lock_release(&b->lock);

    k_spin_lock_acquire(&bcache.lock);
    b->refcnt--;
    if (b->refcnt == 0) {
        // no one is waiting for it.
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }

    k_spin_lock_release(&bcache.lock);
}

void bpin(buf_t *b) {
    k_spin_lock_acquire(&bcache.lock);
    b->refcnt++;
    k_spin_lock_release(&bcache.lock);
}

void bunpin(buf_t *b) {
    k_spin_lock_acquire(&bcache.lock);
    b->refcnt--;
    k_spin_lock_release(&bcache.lock);
}

void k_sd_read(buf_t *buffers[], uint start_block_id, uint block_num) {
    for (int i = 0; i < block_num; i++) {
        buffers[i] = bread(DEV_VDA2, start_block_id + i);
    }
}

void k_sd_write(buf_t *buffers[], uint block_num) {
    for (int i = 0; i < block_num; i++) {
        bwrite(buffers[i]);
    }
}

void k_sd_release(buf_t *buffers[], uint block_num) {}