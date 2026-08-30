// // // Physical memory allocator, for user processes,
// // // kernel stacks, page-table pages,
// // // and pipe buffers. Allocates whole 4096-byte pages.

// #include "types.h"
// #include "param.h"
// #include "memlayout.h"
// #include "spinlock.h"
// #include "riscv.h"
// #include "defs.h"

// void freerange(void *pa_start, void *pa_end);

// extern char end[]; // first address after kernel.
//                    // defined by kernel.ld.

// static struct  spinlock ref_lock;

// #define NPAGES (PHYSTOP/PGSIZE)

// static int page_refcnt[NPAGES];

// static inline int pa2idx(uint64 pa){
//   return (int)(pa / PGSIZE);
// }

// void page_ref_init(void){

//   initlock(&ref_lock,"page_ref");

//   for(int i = 0;i < NPAGES;i++) page_refcnt[i] = 0;

// }

// void page_ref_inc(uint64 pa){
//   int idx = pa2idx(pa);
//   acquire(&ref_lock);
//   page_refcnt[idx]++;
//   release(&ref_lock);
// }
// int page_ref_dec(uint64 pa){
//   int idx = pa2idx(pa);
//   int newv;

//   acquire(&ref_lock);
  
//   if(page_refcnt[idx] <= 0){
//     release(&ref_lock);

//     panic("page_ref_cnt : refcount <= 0");

//   }

//   page_refcnt[idx]--;
//   newv = page_refcnt[idx];
//   release(&ref_lock);

//   return newv;
// }

// int page_ref_get(uint64 pa){
//   int idx = pa2idx(pa);
//   int v;

//   acquire(&ref_lock);
//   v = page_refcnt[idx];
//   release(&ref_lock);

//   return v;
// }

// // -------- helper functions  defined -----------



// struct run {
//   struct run *next;
// };

// struct {
//   struct spinlock lock;
//   struct run *freelist;
// } kmem;

// void
// kinit()
// {
//   initlock(&kmem.lock, "kmem");
//   page_ref_init();
//   freerange(end, (void*)PHYSTOP);
// }

// void
// freerange(void *pa_start, void *pa_end)
// {
//   char *p;
//   p = (char*)PGROUNDUP((uint64)pa_start);
//   for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
//     kfree(p);
// }

// // Free the page of physical memory pointed at by pa,
// // which normally should have been returned by a
// // call to kalloc().  (The exception is when
// // initializing the allocator; see kinit above.)
// void
// kfree(void *pa)
// {
//   struct run *r;
//   uint64 p;

//   int idx;

//   if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
//     panic("kfree");

//   // Fill with junk to catch dangling refs.
//   p = V2P(pa);
//   idx = pa2idx(p);

//   acquire(&ref_lock);
//   if(page_refcnt[idx] <= 0){
//     release(&ref_lock);
//     panic("kfree : refcount <= 0");
//   }
//   page_refcnt[idx]--;
//   int refs = page_refcnt[idx];
//   release(&ref_lock);

//   if(refs > 0){

//     return ;
//   }

//   memset(pa, 1, PGSIZE);

//   r = (struct run*)pa;

//   acquire(&kmem.lock);
//   r->next = kmem.freelist;
//   kmem.freelist = r;
//   release(&kmem.lock);
// }

// // Allocate one 4096-byte page of physical memory.
// // Returns a pointer that the kernel can use.
// // Returns 0 if the memory cannot be allocated.
// void *
// kalloc(void)
// {
//   struct run *r;

//   acquire(&kmem.lock);
//   r = kmem.freelist;
//   if(r)
//     kmem.freelist = r->next;
//   release(&kmem.lock);

//   if(r){
    
//     uint64 pa = V2P((void *)r);
//     int idx = pa2idx(pa);
//     acquire(&ref_lock);
//     page_refcnt[idx] = 1;
//     release(&ref_lock);

//     memset((char*)r, 5, PGSIZE); // fill with junk

//   }
  
//   return (void*)r;
// }
// // kernel/kalloc.c
// // Physical memory allocator with per-physical-page reference counting
// // for Copy-On-Write (Task 4.1).

// // kernel/kalloc.c
// // Physical memory allocator with per-physical-page reference counting
// // for Copy-On-Write (Task 4.1) — xv6-riscv version (no V2P/P2V).

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *vstart, void *vend);

extern char end[]; // first address after kernel. defined by kernel.ld.

// ---------- refcount data ----------
static struct spinlock ref_lock;
#define NPAGES (PHYSTOP / PGSIZE)
static int page_refcnt[NPAGES];

static inline int
pa2idx(uint64 pa) {
  return (int)(pa / PGSIZE);
}

void
page_ref_init(void) {
  initlock(&ref_lock, "page_ref");
  for (int i = 0; i < NPAGES; i++)
    page_refcnt[i] = 0;
}

void
page_ref_inc(uint64 pa) {
  int idx = pa2idx(pa);
  acquire(&ref_lock);
  page_refcnt[idx]++;
  release(&ref_lock);
}

// return new value after decrement
int
page_ref_dec(uint64 pa) {
  int idx = pa2idx(pa);
  int newv;
  acquire(&ref_lock);
  if (page_refcnt[idx] <= 0) {
    release(&ref_lock);
    panic("page_ref_dec: refcount <= 0");
  }
  page_refcnt[idx]--;
  newv = page_refcnt[idx];
  release(&ref_lock);
  return newv;
}

int
page_ref_get(uint64 pa) {
  int idx = pa2idx(pa);
  int v;
  acquire(&ref_lock);
  v = page_refcnt[idx];
  release(&ref_lock);
  return v;
}

// ---------- free-list data ----------
struct pageinfo{
  uint64 last_used;
  int owner_pid;
  uint64 owner_va;
  int on_free_list;
  int is_kernel;
};

static struct pageinfo pages[NPAGES];

static struct spinlock page_lock;


struct run { 
  struct run *next; 

};

static inline void set_page_owner(uint64 pa,int pid,uint64 va,int is_kernel){

    int idx = pa2idx(pa);
    acquire(&page_lock);
    pages[idx].owner_pid = pid;
    pages[idx].owner_va = va;
    pages[idx].is_kernel = is_kernel;
    pages[idx].on_free_list = 0;
    pages[idx].last_used = 0;
    release(&page_lock);
}

static inline void clear_page_owner(uint64 pa){
  int idx = pa2idx(pa);
  acquire(&page_lock);
  pages[idx].owner_pid = 0;
  pages[idx].owner_va = 0;
  pages[idx].is_kernel = 0;
  pages[idx].on_free_list = 1;
  pages[idx].last_used = 0;
  release(&page_lock);
}



struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// kinit: initialize kmem and refcounts, then add real pages to freelist
void
kinit(void)
{
  initlock(&kmem.lock, "kmem");
  initlock(&page_lock,"page_lock");

  page_ref_init();               // MUST initialize refcounts before freerange
  freerange(end, (void*)PHYSTOP);
}

// freerange: add kernel virtual pages in [vstart, vend) to free list.
// During bootstrap we set refcount = 0 and push pages directly to freelist.
void
freerange(void *vstart, void *vend)
{
  char *p = (char*)PGROUNDUP((uint64)vstart);
  for (; p + PGSIZE <= (char*)vend; p += PGSIZE) {
    uint64 pa = (uint64)p;      // xv6-riscv: kernel VA == physical address
    int idx = pa2idx(pa);

    acquire(&ref_lock);
    page_refcnt[idx] = 0;
    release(&ref_lock);

    struct run *r = (struct run*)p;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
}






// kfree: free a kernel-virtual page pointer 'v'.
// Decrement reference count; only return to freelist when it drops to 0.
void
kfree(void *v)
{
  struct run *r;
  uint64 pa;
  int idx;

  if (((uint64)v % PGSIZE) != 0 || (char*)v < end || (uint64)v >= PHYSTOP)
    panic("kfree");

  // In xv6-riscv kernel VA == physical address
  pa = (uint64)v;
  idx = pa2idx(pa);

  // Decrement refcount and return if still referenced
  acquire(&ref_lock);
  if (page_refcnt[idx] <= 0) {
    release(&ref_lock);
    panic("kfree: refcount <= 0");
  }
  page_refcnt[idx]--;
  int refs = page_refcnt[idx];
  release(&ref_lock);

  if (refs > 0) {
    // page is still shared; don't actually free
    return;
  }

  clear_page_owner(pa);
  // Actually free the page: overwrite for debugging and push onto freelist
  memset(v, 1, PGSIZE);
  r = (struct run*)v;
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// kalloc: pop a page from freelist, set refcount = 1, return kernel virtual addr
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r) {
    uint64 pa = (uint64)r;      // xv6-riscv: kernel VA == physical address
    int idx = pa2idx(pa);
    acquire(&ref_lock);
    page_refcnt[idx] = 1;
    release(&ref_lock);

    acquire(&page_lock);
    pages[idx].last_used = ticks;
    pages[idx].on_free_list = 0;
    pages[idx].owner_pid = 0;
    pages[idx].owner_va = 0;
    pages[idx].is_kernel = 0;
    release(&page_lock);

     
    return (void*)r;
  }

  return 0;
}


// // kernel/kalloc.c
// // Physical memory allocator with per-page reference counting
// // and MRU (Most Recently Used) page replacement policy

// // #include "types.h"
// // #include "param.h"
// // #include "memlayout.h"
// // #include "spinlock.h"
// // #include "riscv.h"
// // #include "defs.h"

// // void freerange(void *vstart, void *vend);
// // extern char end[]; // first address after kernel, defined by kernel.ld

// // // ---------- reference count ----------
// // static struct spinlock ref_lock;
// #define NPAGES (PHYSTOP / PGSIZE)
// static int page_refcnt[NPAGES];

// static inline int pa2idx(uint64 pa) {
//     return (int)(pa / PGSIZE);
// }

// void page_ref_init(void) {
//     initlock(&ref_lock, "page_ref");
//     for (int i = 0; i < NPAGES; i++)
//         page_refcnt[i] = 0;
// }

// void page_ref_inc(uint64 pa) {
//     int idx = pa2idx(pa);
//     acquire(&ref_lock);
//     page_refcnt[idx]++;
//     release(&ref_lock);
// }

// int page_ref_dec(uint64 pa) {
//     int idx = pa2idx(pa);
//     int newv;
//     acquire(&ref_lock);
//     if (page_refcnt[idx] <= 0) {
//         release(&ref_lock);
//         panic("page_ref_dec: refcount <= 0");
//     }
//     page_refcnt[idx]--;
//     newv = page_refcnt[idx];
//     release(&ref_lock);
//     return newv;
// }

// int page_ref_get(uint64 pa) {
//     int idx = pa2idx(pa);
//     int v;
//     acquire(&ref_lock);
//     v = page_refcnt[idx];
//     release(&ref_lock);
//     return v;
// }

// // ---------- page info for MRU ----------
// struct pageinfo {
//     uint64 last_used;
//     int owner_pid;
//     uint64 owner_va;
//     int on_free_list;
//     int is_kernel;
// };

// static struct pageinfo pages[NPAGES];
// static struct spinlock page_lock;

// static inline void set_page_owner(uint64 pa, int pid, uint64 va, int is_kernel) {
//     int idx = pa2idx(pa);
//     acquire(&page_lock);
//     pages[idx].owner_pid = pid;
//     pages[idx].owner_va = va;
//     pages[idx].is_kernel = is_kernel;
//     pages[idx].on_free_list = 0;
//     pages[idx].last_used = ticks;
//     release(&page_lock);
// }

// static inline void clear_page_owner(uint64 pa) {
//     int idx = pa2idx(pa);
//     acquire(&page_lock);
//     pages[idx].owner_pid = 0;
//     pages[idx].owner_va = 0;
//     pages[idx].is_kernel = 0;
//     pages[idx].on_free_list = 1;
//     pages[idx].last_used = 0;
//     release(&page_lock);
// }

// void page_accessed(uint64 pa) {
//     int idx = pa2idx(pa);
//     acquire(&page_lock);
//     pages[idx].last_used = ticks;
//     release(&page_lock);
// }

// // ---------- free list ----------
// struct run { 
//     struct run *next; 
// };

// static struct {
//     struct spinlock lock;
//     struct run *freelist;
// } kmem;

// // kinit: initialize memory allocator
// void kinit(void) {
//     initlock(&kmem.lock, "kmem");
//     initlock(&page_lock, "page_lock");
//     page_ref_init();
//     freerange(end, (void*)PHYSTOP);
// }

// // freerange: add pages to freelist
// void freerange(void *vstart, void *vend) {
//     char *p = (char*)PGROUNDUP((uint64)vstart);
//     for (; p + PGSIZE <= (char*)vend; p += PGSIZE) {
//         uint64 pa = (uint64)p;
//         int idx = pa2idx(pa);

//         acquire(&ref_lock);
//         page_refcnt[idx] = 0;
//         release(&ref_lock);

//         struct run *r = (struct run*)p;
//         acquire(&kmem.lock);
//         r->next = kmem.freelist;
//         kmem.freelist = r;
//         release(&kmem.lock);

//         acquire(&page_lock);
//         pages[idx].on_free_list = 1;
//         pages[idx].last_used = 0;
//         pages[idx].owner_pid = 0;
//         pages[idx].owner_va = 0;
//         pages[idx].is_kernel = 1;
//         release(&page_lock);
//     }
// }

// // ---------- MRU eviction ----------
// static uint64 evict_mru(void) {
//     uint64 mru_ticks = 0;
//     int best_idx = -1;

//     acquire(&page_lock);
//     for (int i = 0; i < NPAGES; i++) {
//         if (!pages[i].on_free_list && pages[i].owner_pid != 0 && 
//             pages[i].is_kernel == 0 && page_ref_get(i * PGSIZE) == 1) {
//             if (pages[i].last_used > mru_ticks) {
//                 mru_ticks = pages[i].last_used;
//                 best_idx = i;
//             }
//         }
//     }
//     release(&page_lock);

//     if (best_idx == -1)
//         return 0;

//     uint64 pa = best_idx * PGSIZE;
//     kfree((void*)pa); // free the MRU page
//     return pa;
// }

// // ---------- free a page ----------
// void kfree(void *v) {
//     struct run *r;
//     uint64 pa = (uint64)v;
//     int idx = pa2idx(pa);

//     if ((pa % PGSIZE) != 0 || (char*)v < end || pa >= PHYSTOP)
//         panic("kfree");

//     acquire(&ref_lock);
//     if (page_refcnt[idx] <= 0) {
//         release(&ref_lock);
//         panic("kfree: refcount <= 0");
//     }
//     page_refcnt[idx]--;
//     int refs = page_refcnt[idx];
//     release(&ref_lock);

//     if (refs > 0)
//         return;

//     clear_page_owner(pa);
//     memset(v, 1, PGSIZE);

//     r = (struct run*)v;
//     acquire(&kmem.lock);
//     r->next = kmem.freelist;
//     kmem.freelist = r;
//     release(&kmem.lock);
// }

// // ---------- allocate a page ----------
// void *kalloc(void) {
//     struct run *r;

//     acquire(&kmem.lock);
//     r = kmem.freelist;
//     if (r)
//         kmem.freelist = r->next;
//     release(&kmem.lock);

//     if (!r) {
//         uint64 victim_pa = evict_mru();
//         if (victim_pa == 0)
//             return 0;
//         r = (struct run*)victim_pa;
//     }

//     uint64 pa = (uint64)r;
//     int idx = pa2idx(pa);

//     acquire(&ref_lock);
//     page_refcnt[idx] = 1;
//     release(&ref_lock);

//     acquire(&page_lock);
//     pages[idx].last_used = ticks;
//     pages[idx].on_free_list = 0;
//     pages[idx].owner_pid = 0;
//     pages[idx].owner_va = 0;
//     pages[idx].is_kernel = 0;
//     release(&page_lock);

//     memset((char*)r, 5, PGSIZE);
//     return (void*)r;
// }
