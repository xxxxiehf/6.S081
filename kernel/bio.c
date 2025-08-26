// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASH_SIZE 13
#define HASH(x) (x % HASH_SIZE)

struct {
    // the actual whole buffer cache
    struct buf buf[NBUF];

    struct spinlock global_lock;

    // lock for each hash bucket
    struct spinlock table_lock[HASH_SIZE];

    struct buf *buckets[HASH_SIZE];
} bcache;

void binit(void) {
    initlock(&bcache.global_lock, "bcache");

    for (int i = 0; i < HASH_SIZE; i++) {
        char *name = "bcache-bucket-00";
        name[14] += (i / 10);
        name[15] += (i % 10);
        initlock(&bcache.table_lock[i], name);

        bcache.buckets[i] = 0;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *bget(uint dev, uint blockno) {
    struct buf *b;

    int bucket_index = HASH(blockno);
    acquire(&bcache.table_lock[bucket_index]);

    for (b = bcache.buckets[bucket_index]; b != 0; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&bcache.table_lock[bucket_index]);
            acquiresleep(&b->lock);
            return b;
        }
    }

    // Not cached
    // Go through the whole array and find the one available with smallest tick
    acquire(&bcache.global_lock);
    int evict_index = 0;
    uint min_tick = __UINT32_MAX__;
    for (int i = 0; i < NBUF; i++) {
        if (bcache.buf[i].refcnt == 0) {
            if (bcache.buf[i].tick < min_tick) {
                evict_index = i;
                min_tick = bcache.buf[i].tick;
                if (min_tick == 0) {
                    break;
                }
            }
        }
    }

    if (evict_index >= NBUF) {
        release(&bcache.global_lock);
        release(&bcache.table_lock[bucket_index]);
        panic("bget: no buffers");
    }

    // we have identified the eviction victim, setting the tick and refcnt so
    // that it won't be chosen after release
    b = &bcache.buf[evict_index];
    b->tick = __UINT32_MAX__;
    b->refcnt = 1;

    // evict if it is in the bucket somewhere
    if (b->blockno != 0) {
        if (b->prev) {
            b->prev->next = b->next;
        }
        if (b->next) {
            b->next->prev = b->prev;
        }
        if (bcache.buckets[HASH(b->blockno)] == b) {
            bcache.buckets[HASH(b->blockno)] = b->next;
        }
    }
    release(&bcache.global_lock);

    b->valid = 0;
    b->dev = dev;
    b->blockno = blockno;
    b->prev = 0;
    b->next = bcache.buckets[bucket_index];
    if (bcache.buckets[bucket_index]) {
        bcache.buckets[bucket_index]->prev = b;
    }
    bcache.buckets[bucket_index] = b;

    release(&bcache.table_lock[bucket_index]);
    acquiresleep(&b->lock);

    return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf *bread(uint dev, uint blockno) {
    struct buf *b;

    b = bget(dev, blockno);
    if (!b->valid) {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b) {
    if (!holdingsleep(&b->lock)) {
        panic("bwrite");
    }
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
void brelse(struct buf *b) {
    if (!holdingsleep(&b->lock)) {
        panic("brelse");
    }

    releasesleep(&b->lock);

    acquire(&tickslock);
    b->tick = ticks;
    release(&tickslock);
    b->refcnt--;
}

void bpin(struct buf *b) {
    int index = HASH(b->blockno);
    acquire(&bcache.table_lock[index]);
    b->refcnt++;
    release(&bcache.table_lock[index]);
}

void bunpin(struct buf *b) {
    int index = HASH(b->blockno);
    acquire(&bcache.table_lock[index]);
    b->refcnt--;
    release(&bcache.table_lock[index]);
}
