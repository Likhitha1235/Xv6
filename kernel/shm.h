// kernel/shm.h
#ifndef XV6_SHM_H
#define XV6_SHM_H

#include "types.h"
#include "spinlock.h"

#define MAX_SHM_REGIONS 64
#define SHM_KEY_UNUSED 0

struct shm_region {
  int key;            // unique key
  void *pa;           // physical address (page)
  int count;         // number of processes currently attached
  int inuse;          // 0 = free, 1 = used
};

extern struct spinlock shm_lock;
extern struct shm_region shm_table[MAX_SHM_REGIONS];

#endif
