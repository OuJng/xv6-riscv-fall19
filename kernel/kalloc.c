// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);
void freerangeInit(void *pa_start, void *pa_end);
void kfreeInit(void *pa);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint *refs;
} kmem;


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.refs = (uint*)end;

  // Length of the array
  uint64 refLength = ((PHYSTOP - (uint64)end) >> 12) + 1;
  // // how many pages should use
  // uint64 refPageOffset = refLength * (sizeof(uint) >> 12) + 1;
  // uint64 physicalOffset = refPageOffset << 12;
  uint64 physicalOffset = refLength * sizeof(uint);

  printf("debug, len %d\n", refLength);
  memset(kmem.refs, 0, refLength * (sizeof(uint) / sizeof(char)));

  freerangeInit(end + physicalOffset, (void*)PHYSTOP);
}



void kRefIncrease(uint64 pa) {
  uint64 index = ((uint64)pa - (uint64)end) >> 12;

  acquire(&kmem.lock);
  kmem.refs[index]++;
  release(&kmem.lock);
}


// replace with kfree. Same effect.
// void kRefDecrease(void *pa) {
//   uint64 index = ((uint64)pa - (uint64)end) >> 12;
//   uint8 isZero = 0;

//   acquire(&kmem.lock);
//   if(--kmem.refs[index] == 0)
//     isZero = 1;
//   if(kmem.refs[index] < 0)
//     panic("kalloc: negative ref");
//   release(&kmem.lock);

//   if(isZero) {
//     kfree(pa);
//   }
// }


void freerangeInit(void *pa_start, void *pa_end)
{
  char *p;

  p = (char*)PGROUNDUP((uint64)pa_start);

  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfreeInit(p);
  
}


void
freerange(void *pa_start, void *pa_end)
{
  char *p;

  p = (char*)PGROUNDUP((uint64)pa_start);

  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
  
}


void kfreeInit(void *pa)
{
  struct run *r;

  // if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
  //   panic("kfree");

  // uint64 index = ((uint64)pa - (uint64)end) >> 12;

  // acquire(&kmem.lock);
  // if(--kmem.refs[index] != 0) {     // still have reference, don't free
  //   release(&kmem.lock);
  //   return;
  // }
  // release(&kmem.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  // release(&kmem.lock);
}


// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  uint64 index = ((uint64)pa - (uint64)end) >> 12;

  acquire(&kmem.lock);
  if((--kmem.refs[index]) != 0) {     // still have reference, don't free
    release(&kmem.lock);
    return;
  }
  release(&kmem.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  uint64 index = ((uint64)r - (uint64)end) >> 12;
  if(r) {
    kmem.freelist = r->next;
    // // debug
    // if(kmem.refs[index]) {
    //   panic("kalloc: ref error");
    // }
    kmem.refs[index] = 1;
  }
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}
