#pragma once

#include <asm/pgtable.h>
#include <common/types.h>
#include <os/lock.h>

#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

#define VIRTIO_MMIO_MAGIC_VALUE 0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION 0x004     // version; should be 2
#define VIRTIO_MMIO_DEVICE_ID 0x008   // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID 0x00c   // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_QUEUE_SEL 0x030        // select queue, write-only
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034    // max size of current queue, read-only
#define VIRTIO_MMIO_QUEUE_NUM 0x038        // size of current queue, write-only
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
#define VIRTIO_F_ANY_LAYOUT 27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX 29

// this many virtio descriptors.
// must be a power of two.
#define DESC_NUM 8
#define BSIZE 512
#define BNUM 1600

#define MAXOPBLOCKS 10            // max # of blocks any FS op writes
#define LOGSIZE (MAXOPBLOCKS * 3) // max data blocks in on-disk log
#define NBUF (MAXOPBLOCKS * 3)    // size of disk block cache

#define DEV_INV 0
#define DEV_VDA2 1

extern uintptr_t virtio_base;

#define R(r) ((volatile uint32 *)(virtio_base + (r)))

typedef struct buf {
    int valid; // has data been read from disk?
    int disk;  // does disk "own" buf?
    uint dev;
    uint blockno;
    sleep_lock_t lock;
    uint refcnt;
    struct buf *prev; // LRU cache list
    struct buf *next;
    uint8_t data[BSIZE];
} buf_t;

// a single descriptor, from the spec.
typedef struct virtq_desc {
    uint64 addr;
    uint32 len;
    uint16 flags;
    uint16 next;
} virtq_desc_t;
#define VRING_DESC_F_NEXT 1  // chained with another descriptor
#define VRING_DESC_F_WRITE 2 // device writes (vs read)

// the (entire) avail ring, from the spec.
typedef struct virtq_avail {
    uint16 flags;          // always zero
    uint16 idx;            // driver will write ring[idx] next
    uint16 ring[DESC_NUM]; // descriptor numbers of chain heads
    uint16 unused;
} virtq_avail_t;

// one entry in the "used" ring, with which the
// device tells the driver about completed requests.
typedef struct virtq_used_elem {
    uint32 id; // index of start of completed descriptor chain
    uint32 len;
} virtq_used_elem_t;

typedef struct virtq_used {
    uint16 flags; // always zero
    uint16 idx;   // device increments when it adds a ring[] entry
    virtq_used_elem_t ring[DESC_NUM];
} virtq_used_t;

// these are specific to virtio block devices, e.g. disks,
// described in Section 5.2 of the spec.

#define VIRTIO_BLK_T_IN 0  // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

// the format of the first descriptor in a disk request.
// to be followed by two more descriptors containing
// the block, and a one-byte status.
typedef struct virtio_blk_req {
    uint32 type; // VIRTIO_BLK_T_IN or ..._OUT
    uint32 reserved;
    uint64 sector;
} virtio_blk_req_t;

typedef struct disk {
    // a set (not a ring) of DMA descriptors, with which the
    // driver tells the device where to read and write individual
    // disk operations. there are DESC_NUM descriptors.
    // most commands consist of a "chain" (a linked list) of a couple of
    // these descriptors.
    char pages[2 * NORMAL_PAGE_SIZE];
    virtq_desc_t *desc;

    // a ring in which the driver writes descriptor numbers
    // that the driver would like the device to process.  it only
    // includes the head descriptor of each chain. the ring has
    // DESC_NUM elements.
    virtq_avail_t *avail;

    // a ring in which the device writes descriptor numbers that
    // the device has finished processing (just the head of each chain).
    // there are DESC_NUM used ring entries.
    virtq_used_t *used;

    // our own book-keeping.
    char free[DESC_NUM]; // is a descriptor free?
    uint16 used_idx;     // we've looked this far in used[2..DESC_NUM].

    // track info about in-flight operations,
    // for use when completion interrupt arrives.
    // indexed by first descriptor index of chain.
    struct {
        buf_t *b;
        char status;
    } info[DESC_NUM];

    // disk command headers.
    // one-for-one with descriptors, for convenience.
    virtio_blk_req_t ops[DESC_NUM];

    spin_lock_t vdisk_lock;

} disk_t;

extern disk_t disk;

void virtio_disk_init(void);

void binit(void);
buf_t *bread(uint dev, uint blockno);
void brelease(buf_t *);
void bwrite(buf_t *);
void bpin(buf_t *);
void bunpin(buf_t *);

void k_sd_read(buf_t *buffers[], uint *start_block_id, uint block_num);
void k_sd_write(buf_t *buffers[], uint block_num);
void k_sd_release(buf_t *buffers[], uint block_num);

void sys_sd_read();
void sys_sd_write();
void sys_sd_release();