#include <drivers/virtio/virtio.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <os/ioremap.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/pcb.h>
#include <os/smp.h>

disk_t __attribute__((aligned(NORMAL_PAGE_SIZE))) disk;
uintptr_t virtio_base;

void virtio_disk_init(void) {
    virtio_base = (uintptr_t)ioremap((uint64_t)VIRTIO0, 0x4000 * NORMAL_PAGE_SIZE);
    uint32 status = 0;

    k_spin_lock_init(&disk.vdisk_lock);

    if (*R(VIRTIO_MMIO_MAGIC_VALUE) != VIRTIO_MAGIC || 
        *R(VIRTIO_MMIO_VERSION) != VIRTIO_VERSION || 
        *R(VIRTIO_MMIO_DEVICE_ID) != VIRTIO_DEVICE_ID_BLK || 
        *R(VIRTIO_MMIO_VENDOR_ID) != VIRTIO_VENDOR_ID_QEMU) {
        panic("could not find virtio disk");
    }

    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    *R(VIRTIO_MMIO_STATUS) = status;

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

    // tell device we're completely ready.
    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    *R(VIRTIO_MMIO_STATUS) = status;

    *R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = NORMAL_PAGE_SIZE;

    // initialize queue 0.
    *R(VIRTIO_MMIO_QUEUE_SEL) = 0;
    uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0) {
        panic("virtio disk has no queue 0");
    }
    if (max < DESC_NUM) {
        panic("virtio disk max queue too short");
    }
    *R(VIRTIO_MMIO_QUEUE_NUM) = DESC_NUM;
    k_memset(disk.pages, 0, sizeof(disk.pages));
    *R(VIRTIO_MMIO_QUEUE_PFN) = ((uint64)disk.pages) >> NORMAL_PAGE_SHIFT;

    disk.desc = (vring_desc_t *)disk.pages;
    disk.avail = (uint16 *)(((char *)disk.desc) + DESC_NUM * sizeof(vring_desc_t));
    disk.used = (vring_used_area_t *)(disk.pages + NORMAL_PAGE_SIZE);

    for (int i = 0; i < DESC_NUM; i++) {
        disk.free[i] = 1;
    }

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
    disk.desc[i].addr = 0;
    disk.free[i] = 1;
    k_wakeup(&disk.free[0]);
}

// free a chain of descriptors.
static void free_chain(int i) {
    while (1) {
        free_desc(i);
        if (disk.desc[i].flags & VRING_DESC_F_NEXT) {
            i = disk.desc[i].next;
        } else {
            break;
        }
    }
}

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
    uint64 sector = b->sectorno;

    k_spin_lock_acquire(&disk.vdisk_lock);

    // the spec says that legacy block operations use three
    // descriptors: one for type/reserved/sector, one for
    // the data, one for a 1-byte status result.

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

    struct virtio_blk_outhdr {
        uint32 type;
        uint32 reserved;
        uint64 sector;
    } buf0;

    if (write) {
        buf0.type = VIRTIO_BLK_T_OUT; // write the disk
    } else {
        buf0.type = VIRTIO_BLK_T_IN; // read the disk
    }
    buf0.reserved = 0;
    buf0.sector = sector;

    // buf0 is on a kernel stack, which is not direct mapped,
    // thus the call to kvmpa().
    disk.desc[idx[0]].addr = (uint64)kva2pa((uint64)&buf0);
    disk.desc[idx[0]].len = sizeof(buf0);
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

    disk.info[idx[0]].status = 0;
    disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    // record buf_t for virtio_disk_intr().
    b->disk = 1;
    disk.info[idx[0]].b = b;

    // avail[0] is flags
    // avail[1] tells the device how far to look in avail[2...].
    // avail[2...] are desc[] indices the device should process.
    // we only tell device the first index in our chain of descriptors.
    disk.avail[2 + (disk.avail[1] % DESC_NUM)] = idx[0];
    __sync_synchronize();
    disk.avail[1] = disk.avail[1] + 1;

    k_spin_lock_release(&disk.vdisk_lock);
    k_unlock_kernel();
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number

    // Wait for virtio_disk_intr() to say request has finished.
    while (b->disk == 1) {
        // k_sleep(b, &disk.vdisk_lock);
        __sync_synchronize();
    }
    k_lock_kernel();
    k_spin_lock_acquire(&disk.vdisk_lock);

    disk.info[idx[0]].b = 0;
    free_chain(idx[0]);

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
        b->refcnt = 0;
        b->sectorno = ~0;
        b->dev = ~0;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        k_sleep_lock_init(&b->lock);
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static buf_t *bget(uint dev, uint sectorno) {
    buf_t *b = NULL;

    k_spin_lock_acquire(&bcache.lock);

    // Is the block already cached?
    for (b = bcache.head.next; b != &bcache.head; b = b->next) {
        if (b->dev == dev && b->sectorno == sectorno) {
            b->refcnt++;
            k_spin_lock_release(&bcache.lock);
            // k_sleep_lock_acquire(&b->lock);
            return b;
        }
    }

    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if (b->refcnt == 0) {
            b->dev = dev;
            b->sectorno = sectorno;
            b->valid = 0;
            b->refcnt = 1;
            k_spin_lock_release(&bcache.lock);
            // k_sleep_lock_acquire(&b->lock);
            return b;
        }
    }
    panic("bget: no buffers");
    return b;
}

// Return a locked buf with the contents of the indicated block.
buf_t *bread(uint dev, uint sectorno) {
    buf_t *b;

    b = bget(dev, sectorno);
    if (!b->valid) {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }

    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(buf_t *b) {
    // if (!k_sleep_lock_hold(&b->lock)) {
    //     panic("bwrite");
    // }
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(buf_t *b) {
    // if (!k_sleep_lock_hold(&b->lock)) {
    //     panic("brelse");
    // }

    // k_sleep_lock_release(&b->lock);

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

void k_sd_read(char *buffers, uint *start_block_ids, uint block_num) {
    buf_t *buf;
    for (int i = 0; i < block_num; i++) {
        buf = bread(DEV_VDA2, start_block_ids[i]);
        k_memcpy((uint8_t *)(buffers + i * BSIZE), (uint8_t *)buf->data, BSIZE);
        brelse(buf);
    }
}

void k_sd_write(char *buffers, uint *start_block_ids, uint block_num) {
    buf_t *buf;
    for (int i = 0; i < block_num; i++) {
        buf = bread(DEV_VDA2, start_block_ids[i]);
        k_memcpy((uint8_t *)buf->data, (uint8_t *)(buffers + i * BSIZE), BSIZE);
        bwrite(buf);
        brelse(buf);
    }
}