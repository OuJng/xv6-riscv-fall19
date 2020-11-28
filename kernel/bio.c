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

#define NBUCKET 13
#define BUCKETLEN (NBUF / NBUCKET)

// struct bcache {
//   struct spinlock lock;
//   struct buf buf[BUCKETLEN];
//   // Linked list of all buffers, through prev/next.
//   // head.next is most recently used.
//   struct buf head;
// };

struct buf bbuf[NBUF];

struct bcache {
  struct spinlock lock;
  struct buf head;
};

struct bcache cachebucket[NBUCKET];

void
binit(void)
{
  struct buf *b = bbuf;

  for(int i = 0; i < NBUCKET; i++) {
    struct bcache *bucket = cachebucket + i;
    initlock(&bucket->lock, "bcache");
    // Create linked list of buffers
    bucket->head.prev = &bucket->head;
    bucket->head.next = &bucket->head;
    for(int j = 0; j < BUCKETLEN; j++){
      b->next = bucket->head.next;
      b->prev = &bucket->head;
      initsleeplock(&b->lock, "buffer");
      bucket->head.next->prev = b;
      bucket->head.next = b;
      b++;
    }
  }
  // put the rest into cachebucket[0]
  while(b < bbuf + NBUF) {
    struct bcache *bucket = cachebucket;
    b->next = bucket->head.next;
    b->prev = &bucket->head;
    initsleeplock(&b->lock, "buffer");
    bucket->head.next->prev = b;
    bucket->head.next = b;
    b++;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  struct bcache *bucket = &cachebucket[blockno % NBUCKET];

  acquire(&bucket->lock);

  // Is the block already cached?
  for(b = bucket->head.next; b != &bucket->head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  for(b = bucket->head.prev; b != &bucket->head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // No unused buffer. Steal from another bucket
  for(int i = (blockno + 1) % NBUCKET; i != (blockno % NBUCKET); i = (i+1) % NBUCKET) {
    struct bcache *target = &cachebucket[i];
    if(!holding(&target->lock)) {
      acquire(&target->lock);
      for(b = target->head.next; b != &target->head; b = b->next) {
        if(b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          b->next->prev = b->prev;
          b->prev->next = b->next;
          b->next = bucket->head.next;
          b->prev = &bucket->head;
          bucket->head.next->prev = b;
          bucket->head.next = b;
          release(&bucket->lock);
          release(&target->lock);
          acquiresleep(&b->lock);
          return b;
        }
      }
    }
    release(&target->lock);
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  struct bcache *bucket = cachebucket + (b->blockno % NBUCKET);

  acquire(&bucket->lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bucket->head.next;
    b->prev = &bucket->head;
    bucket->head.next->prev = b;
    bucket->head.next = b;
  }
  release(&bucket->lock);
}

void
bpin(struct buf *b) {
  struct bcache *bucket = cachebucket + (b->blockno % NBUCKET);
  acquire(&bucket->lock);
  b->refcnt++;
  release(&bucket->lock);
}

void
bunpin(struct buf *b) {
  struct bcache *bucket = cachebucket + (b->blockno % NBUCKET);
  acquire(&bucket->lock);
  b->refcnt--;
  release(&bucket->lock);
}


