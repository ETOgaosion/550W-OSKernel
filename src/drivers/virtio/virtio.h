#pragma once

#include <asm/pgtable.h>
#include <common/types.h>
#include <os/lock.h>

#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

#define VIRTIO_MAGIC 0x74726976
#define VIRTIO_VERSION 0x1
#define VIRTIO_DEVICE_ID_INV 0x0
#define VIRTIO_DEVICE_ID_NET 0x1
#define VIRTIO_DEVICE_ID_BLK 0x2
#define VIRTIO_VENDOR_ID_QEMU 0x554d4551

#define VIRTIO_MMIO_MAGIC_VALUE 0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION 0x004     // version; should be 2
#define VIRTIO_MMIO_DEVICE_ID 0x008   // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID 0x00c   // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028  // page size for PFN, write-only
#define VIRTIO_MMIO_QUEUE_SEL 0x030        // select queue, write-only
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034    // max size of current queue, read-only
#define VIRTIO_MMIO_QUEUE_NUM 0x038        // size of current queue, write-only
#define VIRTIO_MMIO_QUEUE_PFN 0x040        // physical page number for queue, read/write
#define VIRTIO_MMIO_QUEUE_READY 0x044      // ready bit
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050     // write-only
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060 // read-only
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064    // write-only
#define VIRTIO_MMIO_STATUS 0x070           // read/write
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080   // physical address for descriptor table, write-only
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_DRIVER_DESC_LOW 0x090 // physical address for available ring, write-only
#define VIRTIO_MMIO_DRIVER_DESC_HIGH 0x094
#define VIRTIO_MMIO_DEVICE_DESC_LOW 0x0a0 // physical address for used ring, write-only
#define VIRTIO_MMIO_DEVICE_DESC_HIGH 0x0a4

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE 1
#define VIRTIO_CONFIG_S_DRIVER 2
#define VIRTIO_CONFIG_S_DRIVER_OK 4
#define VIRTIO_CONFIG_S_FEATURES_OK 8

// device feature bits
#define VIRTIO_BLK_F_RO 5          /* Disk is read-only */
#define VIRTIO_BLK_F_SCSI 7        /* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE 11 /* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ 12         /* support more than one vq */

#define VIRTIO_BLK_T_IN 0  // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

#define VIRTIO_F_ANY_LAYOUT 27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX 29

#define VRING_DESC_F_NEXT 1  // chained with another descriptor
#define VRING_DESC_F_WRITE 2 // device writes (vs read

// this many virtio descriptors.
// must be a power of two.
#define DESC_NUM 8
#define BSIZE 512
#define BNUM 1600

#define MAXOPBLOCKS 10            // max # of blocks any FS op writes
#define LOGSIZE (MAXOPBLOCKS * 3) // max data blocks in on-disk log
#define NBUF (MAXOPBLOCKS * 3)    // size of disk block cache

#define DEV_VDA2 0

extern uintptr_t virtio_base;

#define R(r) ((volatile uint32 *)(virtio_base + (r)))

typedef struct buf {
    int valid;
    int disk; // does disk "own" buf?
    uint dev;
    uint sectorno; // sector number
    sleep_lock_t lock;
    uint refcnt;
    struct buf *prev;
    struct buf *next;
    char data[BSIZE];
} buf_t;

typedef struct vring_desc_t {
    uint64 addr;
    uint32 len;
    uint16 flags;
    uint16 next;
} vring_desc_t;

typedef struct vring_used_elem {
    uint32 id; // index of start of completed descriptor chain
    uint32 len;
} vring_used_elem_t;

typedef struct vring_used_area {
    uint16 flags;
    uint16 id;
    vring_used_elem_t elems[DESC_NUM];
} vring_used_area_t;

typedef struct disk {
    // memory for virtio descriptors &c for queue 0.
    // this is a global instead of allocated because it must
    // be multiple contiguous pages, which kalloc()
    // doesn't support, and page aligned.
    char pages[2 * NORMAL_PAGE_SIZE];
    vring_desc_t *desc;
    uint16 *avail;
    vring_used_area_t *used;

    // our own book-keeping.
    char free[DESC_NUM]; // is a descriptor free?
    uint16 used_idx;     // we've looked this far in used[2..NUM].

    // track info about in-flight operations,
    // for use when completion interrupt arrives.
    // indexed by first descriptor index of chain.
    struct {
        struct buf *b;
        char status;
    } info[DESC_NUM];

    spin_lock_t vdisk_lock;

} disk_t;

extern disk_t disk;

void d_virtio_disk_init(void);
void d_virtio_disk_rw(struct buf *b, int write);
void d_virtio_disk_intr(void);

void d_binit(void);
struct buf *d_bread(uint, uint);
void d_brelse(struct buf *);
void d_bwrite(struct buf *);

void d_sd_read(char *buffers, uint *start_block_id, uint block_num);
void d_sd_write(char *buffers, uint *start_block_ids, uint block_num);