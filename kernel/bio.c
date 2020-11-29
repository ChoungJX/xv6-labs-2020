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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache[NBUCKET];

struct spinlock access_lock;

void
binit(void)
{
  initlock(&access_lock, "bcache");

  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache[i].lock, "bcache_bucket");
  }

  // deliver all of block to first bucket
  for(int i = 0; i < NBUF; i++){
    bcache[0].buf[i].used = 1;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bucket_number = blockno % NBUCKET;

  acquire(&bcache[bucket_number].lock);

  // Is the block already cached?
  for(int i = 0; i < NBUF; i++){
    b = &bcache[bucket_number].buf[i];
    if(b->used == 1){
      if(b->dev == dev && b->blockno == blockno){
        b->refcnt++;
        b->time = ticks;
        release(&bcache[bucket_number].lock);
        // release(&access_lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
  }
  release(&bcache[bucket_number].lock);  // temporary release it to allow other CPUs to acquire.(enhance performance)

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  
  struct buf *b_remove = 0;
  int b_remove_bucket_number = -1;

  uint min_time = ticks + ticks;
  for(int i = 0; i < NBUCKET; i++){
    for(int j = 0; j < NBUF; j++){
      if(bcache[i].buf[j].used == 1 && 
         bcache[i].buf[j].time < min_time && 
         bcache[i].buf[j].refcnt == 0){
        b_remove = &bcache[i].buf[j];
        min_time = b_remove->time;
        b_remove_bucket_number = i;
      }
    }
  }

  if (b_remove_bucket_number == -1){
    panic("bget: no buffers");
  }

  acquire(&access_lock);
  b_remove->used = 0;

  
  acquire(&bcache[bucket_number].lock);  // aquire this lock again (see line 78)
  for(int i = 0; i < NBUF; i++){
    if(bcache[bucket_number].buf[i].used == 0){
      b = &bcache[bucket_number].buf[i];
      break;
    }
  }

  b->used = 1;
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->time = ticks;
    
  release(&bcache[bucket_number].lock);
  release(&access_lock);

  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
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
  virtio_disk_rw(b, 1);
}

// Find bucket id
int
findbkt(struct buf* b){
  for(int i = 0; i < NBUCKET; i++){
    if(b >= bcache[i].buf && b <= &bcache[i].buf[NBUF - 1])
      return i;
  }
  panic("findbkt: error address");
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int number = findbkt(b);
  acquire(&bcache[number].lock);
  b->refcnt--;
  release(&bcache[number].lock);
}

void
bpin(struct buf *b) {
  int number = findbkt(b);
  acquire(&bcache[number].lock);
  b->refcnt++;
  release(&bcache[number].lock);
}

void
bunpin(struct buf *b) {
  int number = findbkt(b);
  acquire(&bcache[number].lock);
  b->refcnt--;
  release(&bcache[number].lock);
}


