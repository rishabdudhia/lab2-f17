#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
//you write this
  acquire(&(shm_table.lock));
  int i = 0; 
  int firstZero = -1;
  int found = 0;
  struct cpu* c = mycpu();
  int a = PGROUNDUP(c->proc->sz);
  pde_t* pgdir = c->proc->pgdir;
  while (i < 64) {
    if (id == shm_table.shm_pages[i].id) {
      found = 1;
      id = i;
      break;
    }
    if (shm_table.shm_pages[i].id == 0 && firstZero < 0) {
      firstZero = i;
    }
    ++i;
  }

  if (!found && firstZero >= 0) {
    shm_table.shm_pages[firstZero].id = id;
    shm_table.shm_pages[firstZero].frame = kalloc();
    memset(shm_table.shm_pages[firstZero].frame, 0, PGSIZE);
    mappages(pgdir, (char*)a, PGSIZE, V2P(shm_table.shm_pages[firstZero].frame), PTE_W|PTE_U);
    shm_table.shm_pages[firstZero].refcnt = 1;
  }
  else if (found) {
    mappages(pgdir, (char*)a, PGSIZE, V2P(shm_table.shm_pages[id].frame), PTE_W|PTE_U);
    ++shm_table.shm_pages[id].refcnt;
  }
  *pointer = (char*)c->proc->sz;
  c->proc->sz = c->proc->sz + PGSIZE;
  release(&(shm_table.lock));
  return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
//you write this too!

  acquire(&(shm_table.lock));
  int i = 0;
  int found = 0;
  while (i < 64) {
    if (id == shm_table.shm_pages[i].id) {
      found = 1;
      id = i;
      break;
    }
  }
  if (found) {
    if (shm_table.shm_pages[id].refcnt != 0) {
      --shm_table.shm_pages[id].refcnt;
    }
    else {
      shm_table.shm_pages[id].id = 0;
      shm_table.shm_pages[id].frame = 0;
    }
  }
  release(&(shm_table.lock));
  return 0; //added to remove compiler warning -- you should decide what to return
}
